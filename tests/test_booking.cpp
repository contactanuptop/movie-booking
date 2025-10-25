#define MINI_CATCH_MAIN
#include "../include/BookingService.hpp"
#include "../third_party/catch_amalgamated.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <algorithm>
#include <random>

using namespace booking;

namespace {
    // Small helpers to avoid hard-coding seat count/labels in tests
    std::vector<std::string> makeSeatLabels() {
        std::vector<std::string> labels;
        labels.reserve(BookingService::TOTAL_SEATS);
        for (int i = 0; i < BookingService::TOTAL_SEATS; ++i) {
            labels.push_back(BookingService::seatLabelFromIndex(i));
        }
        return labels;
    }

    bool verbose() {
        const char* v = std::getenv("TEST_VERBOSE");
        return v && *v;
    }
}

TEST_CASE("Basic booking flow works") {
    BookingService svc;
    int movieId = svc.addMovie("Inception");
    int theaterId = svc.addTheater("Cineplex");
    auto showId = svc.createShow(movieId, theaterId);

    auto available = svc.getAvailableSeats(showId);
    REQUIRE(static_cast<int>(available.size()) == BookingService::TOTAL_SEATS);
    //REQUIRE(available.size() == 20);

    bool ok = svc.bookSeats(showId, {"A1", "A2"});
    REQUIRE(ok);

    auto after = svc.getAvailableSeats(showId);
    REQUIRE(static_cast<int>(after.size()) == BookingService::TOTAL_SEATS - 2);
    //REQUIRE(after.size() == 18);
}

TEST_CASE("Invalid seat booking returns false") {
    BookingService svc;
    int m = svc.addMovie("Matrix");
    int t = svc.addTheater("IMAX");
    auto showId = svc.createShow(m, t);

    // Instead of expecting an exception, just ensure it returns false
    bool ok = svc.bookSeats(showId, {"Z9"});
    REQUIRE(!ok);  // booking should fail for invalid seat
}

TEST_CASE("Concurrent booking: no double-booking occurs") {
    BookingService svc;
    int m = svc.addMovie("Avengers");
    int t = svc.addTheater("PVR");
    auto showId = svc.createShow(m, t);

    std::atomic<int> successCount{0};
    auto tryBook = [&]() {
        if (svc.bookSeats(showId, {"A1"}))  // All threads try to book the same seat.
            successCount++;
    };

    std::vector<std::thread> threads;
    threads.reserve(10);
    for (int i = 0; i < 10; ++i)
        threads.emplace_back(tryBook);
    for (auto& th : threads)
        th.join();

    REQUIRE(successCount == 1); // Only one thread should succeed

    // Integrity: booked + remaining == total
    auto remaining = svc.getAvailableSeats(showId);
    REQUIRE(static_cast<int>(remaining.size()) == BookingService::TOTAL_SEATS - successCount.load());
}

extern int g_thread_count;

TEST_CASE("Concurrency stress test: configurable thread count") {
    BookingService svc;
    int m = svc.addMovie("Avatar");
    int t = svc.addTheater("Grand");
    auto showId = svc.createShow(m, t);

    int threadCount = g_thread_count > 0 ? g_thread_count : 50;
    std::atomic<int> successCount{0};

    // Generate and shuffle seats to spread contention
    auto seats = makeSeatLabels();
    {
        std::mt19937 rng{12345}; // deterministic for tests
        std::shuffle(seats.begin(), seats.end(), rng);
    }

    std::vector<std::thread> pool;
    pool.reserve(threadCount);
/*
    std::vector<std::string> seats = {
        "A1","A2","A3","A4","A5","A6","A7","A8","A9","A10",
        "A11","A12","A13","A14","A15","A16","A17","A18","A19","A20"
    };
*/
    auto worker = [&](int threadId) {
        const std::string& seat = seats[threadId % seats.size()];
        if (svc.bookSeats(showId, {seat}))
            successCount++;
    };

    for (int i = 0; i < threadCount; ++i)
        pool.emplace_back(worker, i);
    for (auto& th : pool)
        th.join();

    auto remaining = svc.getAvailableSeats(showId);
    REQUIRE(remaining.size() == 20 - successCount);
    std::cout << "Threads: " << threadCount
              << " | Successful: " << successCount.load()
              << " | Remaining: " << remaining.size() << std::endl;
}

//ToDO: Add more tests for edge cases, invalid inputs, listing functions, etc.
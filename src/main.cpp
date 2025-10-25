#include "BookingService.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <csignal>
#include <cctype>

using namespace booking;

static std::atomic<bool> g_exitRequested{false};

// -------------------------------------------------------------
// Graceful Ctrl+C handler
// -------------------------------------------------------------
void handleSigInt(int) {
    g_exitRequested = true;
    std::cout << "\n\nReceived Ctrl+C — shutting down gracefully...\n";
}

// -------------------------------------------------------------
// Utility helpers
// -------------------------------------------------------------
static inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end   = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

static bool safeReadInt(const std::string& prompt, int& value) {
    std::string line;
    std::cout << prompt;
    if (!std::getline(std::cin, line)) return false;  // handle EOF
    line = trim(line);
    if (line.empty()) return false;

    try {
        value = std::stoi(line);
        return true;
    } catch (...) {
        std::cerr << "Invalid number, please try again.\n";
        return false;
    }
}

static bool safeReadLong(const std::string& prompt, long long& value) {
    std::string line;
    std::cout << prompt;
    if (!std::getline(std::cin, line)) return false;
    line = trim(line);
    if (line.empty()) return false;

    try {
        value = std::stoll(line);
        return true;
    } catch (...) {
        std::cerr << "Invalid number, please try again.\n";
        return false;
    }
}

// -------------------------------------------------------------
// Main CLI
// -------------------------------------------------------------
int main() {
    // Register Ctrl+C signal handler
    std::signal(SIGINT, handleSigInt);

    BookingService service;

    while (!g_exitRequested) {
        std::cout << "\n===== Movie Booking CLI =====\n"
                  << "1. Add Movie\n"
                  << "2. Add Theater\n"
                  << "3. Create Show\n"
                  << "4. List Movies\n"
                  << "5. List Theaters for a Movie\n"
                  << "6. View Available Seats\n"
                  << "7. Book Seats\n"
                  << "8. Exit\n";

        int choice = 0;
        if (!safeReadInt("Select option: ", choice)) {
            if (std::cin.eof() || g_exitRequested) {
                std::cout << "\nGoodbye!\n";
                break;
            }
            continue; // re-prompt on invalid input
        }

        switch (choice) {
        case 1: {
            std::string title;
            std::cout << "Enter movie title: ";
            std::getline(std::cin, title);
            title = trim(title);
            if (title.empty()) {
                std::cerr << "Title cannot be empty.\n";
                continue;
            }
            int id = service.addMovie(title);
            if(id > 0)
                std::cout << "Movie added. ID[" << id << "]:TITLE[" << title << "]\n";
            break;
        }

        case 2: {
            std::string name;
            std::cout << "Enter theater name: ";
            std::getline(std::cin, name);
            name = trim(name);
            if (name.empty()) {
                std::cerr << "Theater name cannot be empty.\n";
                continue;
            }
            int id = service.addTheater(name);
            if(id > 0)
                std::cout << "Theater added. ID[" << id << "]:NAME[" << name << "]\n";
            break;
        }

        case 3: { // Create Show
            auto movies = service.getAllMovies();
            auto theaters = service.getAllTheaters();
            auto shows = service.getAllShows();

            if (movies.empty() || theaters.empty()) {
                std::cerr << "Please add at least one movie and theater before creating a show.\n";
                break;
            }

            if (!shows.empty()) {
                std::cout << "Current Shows:\n";
                for (const auto& s : shows)
                    std::cout << "  Show ID: " << s.id
                              << " | Movie: " << s.movieTitle
                              << " | Theater: " << s.theaterName << "\n";
            }
            std::cout << "\nAvailable Movies:\n";
            for (auto& [id, title] : movies)
                std::cout << "  [" << id << "] " << title << "\n";

            std::cout << "\nAvailable Theaters:\n";
            for (auto& [id, name] : theaters)
                std::cout << "  [" << id << "] " << name << "\n";

            int movieId, theaterId;
            if (!safeReadInt("\nEnter movie ID: ", movieId)) continue;
            if (!safeReadInt("Enter theater ID: ", theaterId)) continue;

            long long sid = service.createShow(movieId, theaterId);
            if (sid > 0)
                std::cout   << "Show created successfully."
                            << " ShowID:[" << sid << "]"
                            << " Movie[" << service.getMovieTitle(movieId)<< "]"
                            << " Theater[" << service.getTheaterName(theaterId)<< "]\n";
            else
                std::cerr << "Show creation failed.\n";
            break;
        }

        case 4:
            service.listMovies();
            break;

        case 5: { // List Theaters for a Movie
            bool isAnyActiveMov = service.listMovies();

            if(isAnyActiveMov == false)
                continue; // No active movies, so no theaters to list

            int movieId;
            if (!safeReadInt("\nEnter movie ID to see theaters: ", movieId)) {
                std::cerr << "Invalid input.\n";
                continue;
            }
            service.listTheatersForMovie(movieId);
            break;
        }

         case 6: { // View Available Seats
            auto shows = service.getAllShows();
            if (shows.empty()) {
                std::cout << "No shows available.\n";
                continue;
            }

            std::cout << "\nCurrent Shows:\n";
            for (const auto& s : shows)
                std::cout << "  Show ID: " << s.id
                          << " | Movie: " << s.movieTitle
                          << " | Theater: " << s.theaterName
                          << " | Available Seats: " << s.availableSeats << "\n";

            break;
        }

        case 7: { // Book Seats
            auto shows = service.getAllShows();
            if (shows.empty()) {
                std::cout << "No shows available.\n";
                continue;
            }

            std::cout << "\nCurrent Shows with Available Seats:\n";
            for (const auto& s : shows) {
                if (s.availableSeats > 0)
                    std::cout << "  Show ID: " << s.id
                              << " | Movie: " << s.movieTitle
                              << " | Theater: " << s.theaterName
                              << " | Available Seats: " << s.availableSeats << "\n";
            }

            long long showId;
            if (!safeReadLong("\nEnter show ID: ", showId))
                continue;

             // Display available + booked seats explicitly for chosen show
            try {
                auto available = service.getAvailableSeats(showId);
                std::cout << "\nAvailable seats (" << available.size() << "): ";
                for (const auto& s : available) std::cout << s << ' ';
                std::cout << "\n";

                if (available.size() < booking::BookingService::TOTAL_SEATS) {
                    std::cout << "Already Booked seats: ";
                    for (int i = 1; i <= booking::BookingService::TOTAL_SEATS; ++i) {
                        std::string seatLabel = booking::BookingService::seatLabelFromIndex(i - 1);
                        if (std::find(available.begin(), available.end(), seatLabel) == available.end())
                            std::cout << seatLabel << ' ';
                    }
                    std::cout << "\n";
                }
            } catch (const std::invalid_argument&) {
                std::cerr << "Invalid show ID.\n";
                continue;
            }

            std::string input;
            std::cout << "Enter seat labels (comma-separated, valid range: A1–A20). Example: A1,A2 → ";
            std::getline(std::cin, input);

            std::vector<std::string> seatsToBook;
            std::stringstream ss(input);
            std::string token;
            while (std::getline(ss, token, ',')) {
                token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
                if (!token.empty()) seatsToBook.push_back(token);
            }

            if (seatsToBook.empty()) {
                std::cerr << "No valid seats entered.\n";
                continue;
            }

            bool success = service.bookSeats(showId, seatsToBook);
            std::cout << (success ? "Booking successful.\n" : "Booking failed.\n");
            break;
        }
        case 8:
            std::cout << "Exiting Movie Booking CLI.\n";
            return 0;

        default:
            std::cerr << "Invalid option. Please choose 1–8.\n";
        }
    }

    std::cout << "Program terminated cleanly.\n";
    return 0;
}
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <atomic>
#include <cstdint>
#include <utility>
#include <functional>

namespace booking {
// ----------------- Booking Service -----------------
class BookingService {
public:
    // Seat labeling constants
    static constexpr char SEAT_ROW = 'A';
    static constexpr int  TOTAL_SEATS = 20;

    // Represents a movie
    struct Movie {
        int id{};
        std::string title;
    };

    // Represents a theater
    struct Theater {
        int id{};
        std::string name;
    };

    // Represents a show of a movie in a theater
    struct Show {
        int movieId{};
        int theaterId{};
        std::vector<bool> seats;   // true = booked, false = available
        mutable std::mutex mtx;    // per-show seat lock
        int availableCount{TOTAL_SEATS}; // cached available seats
    };

    // For getAllShows()
    struct ShowInfo {
        long long id;
        std::string movieTitle;
        std::string theaterName;
        int availableSeats;
    };

    BookingService() = default;

    [[nodiscard]] int addMovie(const std::string& title);                                       // Add movie and returns movie ID
    [[nodiscard]] int addTheater(const std::string& name);                                      // Add theater and returns theater ID
    [[nodiscard]] long long createShow(int movieId, int theaterId);                             // Create show and returns show ID

    [[nodiscard]] std::vector<std::string> getAvailableSeats(long long showId) const;           // Returns list of available seat labels for the show
    [[nodiscard]] bool bookSeats(long long showId, const std::vector<std::string>& seatLabels); // Books the given seats for the show

    bool listMovies() const;                                                        // Lists all active movies, i.e., with at least one show
    void listTheatersForMovie(int movieId) const;                                   // Lists theaters showing the given movie

    [[nodiscard]] std::string getMovieTitle(int movieId) const;                     // Returns movie title or "Unknown Movie"
    [[nodiscard]] std::string getTheaterName(int theaterId) const;                  // Returns theater name or "Unknown Theater"

    static int seatIndexFromLabel(const std::string& label) noexcept;               // Converts label like "A1", "A2", ... to 0-based index
    static std::string seatLabelFromIndex(int idx);                                 // Converts 0-based index to label like "A1", "A2", ...

    [[nodiscard]] std::vector<ShowInfo> getAllShows() const;                       // Returns vector of ShowInfo structs
    [[nodiscard]] std::vector<std::pair<int, std::string>> getAllMovies() const;   // Returns vector of (movieId, movieTitle)
    [[nodiscard]] std::vector<std::pair<int, std::string>> getAllTheaters() const; // Returns vector of (theaterId, theaterName)

private:
    // Composite key hasher for (movieId, theaterId)
    struct PairHash {
        size_t operator()(const std::pair<int, int>& p) const noexcept {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };

    mutable std::shared_mutex mtx_;                             // Protects all below maps

    std::unordered_map<int, Movie> movies_;                      // movieId to Movie struct hash map
    std::unordered_map<int, Theater> theaters_;                  // theaterId to Theater struct hash map
    std::unordered_map<long long, std::shared_ptr<Show>> shows_; // showId to Show ptr hash map

    // 🔹 Optimization maps
    std::unordered_map<std::string, int> movieNameToId_;    // Secondary hash map for Movie duplicate check based on lowercase title.
    std::unordered_map<std::string, int> theaterNameToId_;  // Secondary hash map for Theater duplicate check based on lowercase name.
    std::unordered_map<std::pair<int, int>, long long, PairHash> showLookup_; // Composite key (movieId, theaterId) lookup for shows. Use Custom hasher.
    std::unordered_set<int> activeMovies_;                  // Maintain hash map for active movies, i.e., set of movie IDs with at least one active show.
    std::unordered_map<int, std::unordered_set<int>> movieToTheaters_; //   Map from movieId to set of theaterIds showing that movie.

    std::atomic<int> movieCounter_{0};              // For generating unique movie IDs
    std::atomic<int> theaterCounter_{0};            // For generating unique theater IDs
    std::atomic<long long> showCounter_{0};         // For generating unique show IDs
};

} // namespace booking
#include "BookingService.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <bitset>
#include <cctype>

namespace booking {

// ----------------- Helpers -----------------
/**
 * Converts the input string `s` to lowercase.
 *
 * Uses `std::transform` with a lambda to convert each character to lowercase
 * and returns a new string.
 *
 * @param s Constant reference to the input string.
 * @return Lowercase version of the input string.
 *
 * Time complexity:  O(n)
 * Space complexity: O(n)
 * Single traversal.
 */
static std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return out;
}

/**
 * The function `seatIndexFromLabel` extracts and returns the seat index from a given seat label
 * string.
 *
 * @param label The `label` parameter is a string representing the seat label in a booking system. It
 * is expected to have a specific format where the first character is the seat row identifier and the
 * subsequent characters represent the seat number. The function `seatIndexFromLabel` extracts the seat
 * index from this label by parsing
 *
 * @return The function `BookingService::seatIndexFromLabel` returns an integer value representing the
 * seat index calculated from the input label. If the label is valid and corresponds to a seat index
 * within the specified range, the function returns the calculated seat index. If the label is invalid
 * or out of range, the function returns -1.
 *
 * Time complexity: O(k) (where k = label length, usually small = constant → effectively O(1)).
 * Space complexity: O(1) — uses constant extra variables.
 */
int BookingService::seatIndexFromLabel(const std::string& label) noexcept {
    if (label.size() < 2 || label[0] != SEAT_ROW) return -1;
    int num = 0;
    for (size_t i = 1; i < label.size(); ++i) {
        char c = label[i];
        if (c < '0' || c > '9') return -1;
        num = num * 10 + (c - '0');
        if (num > TOTAL_SEATS) break;
    }
    if (num < 1 || num > TOTAL_SEATS) return -1;
    return num - 1;
}

/**
 * The function `seatLabelFromIndex` generates a seat label based on the given index.
 *
 * @param idx The `idx` parameter in the `BookingService::seatLabelFromIndex` function represents the
 * index of a seat in a seating arrangement.
 *
 * @return A string representing the seat label based on the given index.
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 */
std::string BookingService::seatLabelFromIndex(int idx) {
    std::ostringstream os;
    os << SEAT_ROW << (idx + 1);
    return os.str();
}

// ----------------- Movie / Theater Creation -----------------
/**
 * The `addMovie` function adds a new movie to a booking service while performing a duplicate
 * check based on the movie title.
 *
 * @param title The `title` parameter is a constant reference to a `std::string` representing the title
 * of the movie that is being added to the booking service.
 *
 * @return The `addMovie` function returns the ID of the newly added movie if it was successfully
 * added. If the movie already exists, it returns -1.
 *
 * All unordered_map operations are amortized O(1).
 * Time complexity: O(1) average
 * Space complexity: O(1) additional (new entries in hash tables).
 * Hash lookup + insert
 */
int BookingService::addMovie(const std::string& title) {
    const std::string lowerTitle = toLower(title);

    // Fast O(1) duplicate check
    std::shared_lock slk(mtx_);
    if (movieNameToId_.count(lowerTitle)) {
        std::cerr << "Movie \"" << title << "\" already exists (ID: "
                  << movieNameToId_.at(lowerTitle) << ")\n";
        return -1;
    }
    slk.unlock();

    const int id = ++movieCounter_;
    std::unique_lock unqLock(mtx_);
    movies_.try_emplace(id, Movie{id, title});
    movieNameToId_[lowerTitle] = id;
    return id;
}

/**
 * The `addTheater` function in C++ adds a new theater with a unique ID and name to a booking service,
 * preventing duplicates based on the theater name.
 *
 * @param name The `name` parameter in the `addTheater` method is a `const std::string&` reference,
 * which represents the name of the theater being added to the booking service.
 *
 * @return The `addTheater` function returns an integer value, which is the ID of the newly added
 * theater. If the theater with the provided name already exists, it returns -1.
 *
 * All unordered_map operations are amortized O(1).
 * Time complexity: O(1) average
 * Space complexity: O(1) per entry.
 * Hash lookup + insert
 */
int BookingService::addTheater(const std::string& name) {
    const std::string lowerName = toLower(name);

    std::shared_lock slk(mtx_);
    if (theaterNameToId_.count(lowerName)) {
        std::cerr << "Theater \"" << name << "\" already exists (ID: "
                  << theaterNameToId_.at(lowerName) << ")\n";
        return -1;
    }
    slk.unlock();

    const int id = ++theaterCounter_;
    std::unique_lock unqLock(mtx_);
    theaters_.try_emplace(id, Theater{id, name});
    theaterNameToId_[lowerName] = id;
    return id;
}

// ----------------- Show Management -----------------
/**
 * The `createShow` function in the `BookingService` class creates a new show for a given movie and
 * theater, assigning seats and updating relevant data structures.
 *
 * @param movieId The `movieId` parameter in the `createShow` function represents the unique identifier
 * of the movie for which a show is being created. It is used to associate the show with a specific
 * movie in the booking service.
 * @param theaterId The `theaterId` parameter in the `createShow` function represents the unique
 * identifier of the theater where the show for a particular movie will take place. It is used to
 * associate the show with a specific theater in the booking service system.
 *
 * @return The `createShow` function returns a `long long` value, which is the ID of the newly created
 * show.
 *
 * All map operations are average O(1).
 * Constant initialization of seats (20 seats).
 * Time complexity: O(1) (since TOTAL_SEATS is constant).
 * Space complexity: O(1) for indices + O(TOTAL_SEATS) per show.
 * Composite key lookup
 */
long long BookingService::createShow(int movieId, int theaterId) {
    std::unique_lock unqLock(mtx_);

    if (!movies_.count(movieId)) {
        std::cerr << "Invalid movie ID: " << movieId << '\n';
        return -1;
    }
    if (!theaters_.count(theaterId)) {
        std::cerr << "Invalid theater ID: " << theaterId << '\n';
        return -1;
    }

    const auto key = std::make_pair(movieId, theaterId);
    if (showLookup_.count(key)) {
        std::cerr << "Duplicate show for movie " << movieId
                  << " and theater " << theaterId << '\n';
        return -1;
    }

    long long id = ++showCounter_;
    auto show = std::make_shared<Show>();
    show->movieId = movieId;
    show->theaterId = theaterId;
    show->seats.assign(TOTAL_SEATS, false);
    show->availableCount = TOTAL_SEATS;

    shows_.emplace(id, show);
    showLookup_[key] = id;
    activeMovies_.insert(movieId);
    movieToTheaters_[movieId].insert(theaterId);
    return id;
}

// ----------------- Seat Availability -----------------
/**
 * The function `getAvailableSeats` retrieves the available seats for a given show ID in a booking
 * service, ensuring thread safety with mutex locks.
 *
 * @param showId The `showId` parameter is a unique identifier for a particular show in the booking
 * service. It is used to look up the show details and retrieve information about the available seats
 * for that specific show.
 *
 * @return A vector of strings containing the labels of available seats for the show with the given
 * showId is being returned.
 *
 * Loops over a constant (20 seats).
 * Time complexity: O(TOTAL_SEATS) = O(1) effectively.
 * Space complexity: O(TOTAL_SEATS) = O(1) fixed.
 * Uses cached count.
 */
std::vector<std::string> BookingService::getAvailableSeats(long long showId) const {
    std::shared_lock shrLock(mtx_);
    auto it = shows_.find(showId);
    if (it == shows_.end()) throw std::invalid_argument("Invalid show ID");
    std::shared_ptr<Show> show = it->second;
    shrLock.unlock();

    std::lock_guard<std::mutex> guard(show->mtx);
    std::vector<std::string> available;
    available.reserve(show->availableCount);
    for (int i = 0; i < TOTAL_SEATS; ++i)
        if (!show->seats[i])
            available.emplace_back(seatLabelFromIndex(i));
    return available;
}

// ----------------- Booking -----------------
/**
 * The function `bookSeats` in the BookingService class books seats for a show based on seat labels,
 * checking for validity and availability.
 *
 * @param showId The `showId` parameter in the `bookSeats` function represents the unique identifier of
 * the show for which seats are being booked. It is used to find the specific show in the `shows_` map
 * where the seats will be booked.
 * @param seatLabels The `seatLabels` parameter is a vector of strings that contains the labels of the
 * seats that need to be booked for a particular show. Each string in the vector represents the label
 * of a seat that the user wants to book for the show.
 *
 * @return The `BookingService::bookSeats` function returns a boolean value. It returns `true` if the
 * seats specified by the seatLabels vector were successfully booked for the show identified by the
 * showId. If any error occurs during the booking process (such as invalid seat labels, duplicate
 * seats, or already booked seats), the function will return `false`.
 *
 * Each operation (lookup, set) is O(1).
 * Time complexity: O(k) where k = reqested seats being booked(small).
 * Space complexity: O(TOTAL_SEATS) = O(1) fixed array and bitset.
 */
bool BookingService::bookSeats(long long showId, const std::vector<std::string>& seatLabels) {
    std::shared_lock shrLock(mtx_);
    auto it = shows_.find(showId);
    if (it == shows_.end()) return false;

    std::shared_ptr<Show> show = it->second;
    shrLock.unlock();

    std::lock_guard<std::mutex> guard(show->mtx);

    std::bitset<TOTAL_SEATS> seen;
    int tmpIndices[TOTAL_SEATS];
    int n = 0;

    for (const auto& lbl : seatLabels) {
        int idx = seatIndexFromLabel(lbl);
        if (idx < 0) {
            std::cerr << "Invalid seat: " << lbl << '\n';
            return false;
        }
        if (seen[idx]) {
            std::cerr << "Duplicate seat: " << lbl << '\n';
            return false;
        }
        seen[idx] = true;

        if (show->seats[idx]) {
            std::cerr << "Seat already booked: " << lbl << '\n';
            return false;
        }
        tmpIndices[n++] = idx;
    }

    for (int i = 0; i < n; ++i) {
        show->seats[tmpIndices[i]] = true;
        show->availableCount--; // update cached available count
    }
    return true;
}

// ----------------- Listing -----------------
/**
 * The function `listMovies` lists the movies currently playing in the booking service.
 *
 * @return The `listMovies` function returns a boolean value. It returns `true` if there are movies
 * currently playing and lists them, and `false` if there are no movies currently playing.
 *
 * Time complexity: O(M_active)
 * Space complexity: O(1) extra.
 * Uses cached active set.
 */
bool BookingService::listMovies() const {
    std::shared_lock slk(mtx_);
    if (activeMovies_.empty()) {
        std::cout << "No movies currently playing.\n";
        return false;
    }

    std::cout << "Movies currently playing:\n";
    for (int id : activeMovies_) {
        auto it = movies_.find(id);
        if (it != movies_.end())
            std::cout << "  [" << id << "] " << it->second.title << "\n";
    }
    return true;
}

/**
 * The function `listTheatersForMovie` lists the theaters showing a specific movie based on the movie
 * ID provided.
 *
 * @param movieId The `listTheatersForMovie` method takes an `int` parameter `movieId`, which
 * represents the unique identifier of a movie for which we want to list the theaters showing it.
 *
 * @return If the `movieId` provided is not found in the `movieToTheaters_` map or if the theaters
 * associated with the movie are empty, the message "No theaters found for this movie." will be printed
 * to the console. If theaters are found for the movie, the function will list the theaters showing the
 * movie by printing their IDs and names to the console.
 *
 * Time complexity: O(K) (K = theaters showing this movie).
 * Space complexity: O(1) extra.
 */
void BookingService::listTheatersForMovie(int movieId) const {
    std::shared_lock shrLock(mtx_);
    auto it = movieToTheaters_.find(movieId);   // O(1)
    if (it == movieToTheaters_.end() || it->second.empty()) {
        std::cout << "No theaters found for this movie.\n";
        return;
    }

    std::cout << "Theaters showing \"" << getMovieTitle(movieId) << "\":\n";
    for (int tid : it->second) {                // O(K)
        std::cout << "  [" << tid << "] " << getTheaterName(tid) << "\n";
    }
}

// ----------------- Utility -----------------
/**
 * The function `getMovieTitle` returns the title of a movie based on its ID, or "Unknown Movie" if the
 * ID is not found.
 *
 * @param movieId The `movieId` parameter is an integer that represents the unique identifier of a
 * movie in the `movies_` map within the `BookingService` class.
 *
 * @return The `getMovieTitle` function returns the title of the movie corresponding to the given
 * `movieId` if it exists in the `movies_` map. If the movie is found, it returns the title of the
 * movie; otherwise, it returns "Unknown Movie".
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 * Hash map lookup.
 */
std::string BookingService::getMovieTitle(int movieId) const {
    std::shared_lock shrLock(mtx_);
    auto it = movies_.find(movieId);
    return (it != movies_.end()) ? it->second.title : "Unknown Movie";
}

/**
 * The function `getTheaterName` returns the name of a theater based on its ID, or "Unknown Theater" if
 * the ID is not found.
 *
 * @param theaterId The `theaterId` parameter is an integer value that represents the unique identifier
 * of a theater. This identifier is used to look up the corresponding theater name in the `theaters_`
 * map within the `BookingService` class. If a theater with the specified `theaterId` exists in
 *
 * @return The function `BookingService::getTheaterName` returns the name of the theater corresponding
 * to the given `theaterId`. If a theater with the provided `theaterId` exists in the `theaters_` map,
 * then its name is returned. Otherwise, the function returns "Unknown Theater".
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 * Hash map lookup.
 */
std::string BookingService::getTheaterName(int theaterId) const {
    std::shared_lock shrLock(mtx_);
    auto it = theaters_.find(theaterId);
    return (it != theaters_.end()) ? it->second.name : "Unknown Theater";
}

/**
 * The function `getAllShows` returns a vector of `ShowInfo` objects containing information about all
 * shows in the booking service.
 *
 * @return A vector of `BookingService::ShowInfo` objects is being returned.
 *
 * Time complexity: O(S) (S = number of shows).
 * Space complexity: O(S) for result vector
 * S = number of shows.
 * Linear in show count
 */
std::vector<BookingService::ShowInfo> BookingService::getAllShows() const {
    std::shared_lock shrLock(mtx_);
    std::vector<ShowInfo> info;
    info.reserve(shows_.size());

    for (const auto& [sid, showPtr] : shows_) {
        if (!showPtr) continue;
        info.push_back({
            sid,
            getMovieTitle(showPtr->movieId),
            getTheaterName(showPtr->theaterId),
            showPtr->availableCount
        });
    }
    return info;
}

/**
 * The function `getAllMovies` returns a vector of pairs containing movie IDs and titles from a
 * BookingService object.
 *
 * @return A vector of pairs containing integers and strings representing the IDs and titles of all
 * movies stored in the `movies_` data member of the `BookingService` class.
 *
 * Time complexity: O(M)
 * Space complexity: O(M)
 * M = number of movies.
 * Linear in movie count
 */
std::vector<std::pair<int, std::string>> BookingService::getAllMovies() const {
    std::shared_lock shrLock(mtx_);
    std::vector<std::pair<int, std::string>> result;
    for (const auto& [id, movie] : movies_)
        result.emplace_back(id, movie.title);
    return result;
}

/**
 * The function `getAllTheaters` returns a vector of pairs containing theater IDs and names from a
 * BookingService object.
 *
 * @return The `getAllTheaters` function returns a vector of pairs, where each pair consists of an
 * integer representing the theater ID and a string representing the theater name.
 *
 * Time complexity: O(T)
 * Space complexity: O(T)
 * T = number of theaters.
 * Linear in theater count
 */
std::vector<std::pair<int, std::string>> BookingService::getAllTheaters() const {
    std::shared_lock shrLock(mtx_);
    std::vector<std::pair<int, std::string>> result;
    for (const auto& [id, theater] : theaters_)
        result.emplace_back(id, theater.name);
    return result;
}

} // namespace booking
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <cstdlib>
#include <numeric>
#include <random>
#include <algorithm>
#include <set>

// misc forward declarations
class Team;
class Room;
class R7Room;
class R8Room;

// global variables
std::array<std::array<int, 4>, 24> orders {{
    {0,1,2,3}, {0,1,3,2}, {0,2,1,3}, {0,2,3,1}, {0,3,1,2}, {0,3,2,1},
    {1,0,2,3}, {1,0,3,2}, {1,2,0,3}, {1,2,3,0}, {1,3,0,2}, {1,3,2,0},
    {2,0,1,3}, {2,0,3,1}, {2,1,0,3}, {2,1,3,0}, {2,3,0,1}, {2,3,1,0},
    {3,0,1,2}, {3,0,2,1}, {3,1,0,2}, {3,1,2,0}, {3,2,0,1}, {3,2,1,0}
}};
std::array<int, 28> upd {}; // Universal Pullup Dict
std::array<int, 28> usd {}; // Universal Sandwich Dict


class Team {
public:
    std::string name {};
    int known {};
    int r7_est {};
    int post_r7 {};
    R7Room* r7_room {nullptr};
    R8Room* r8_room {nullptr};

    Team() = default;
    Team(std::string nm, int kn): name {nm}, known {kn} {
        this->set_score(0); // Necessary for teams that skip r7
    };

    void set_score(int score) {
        r7_est = score;
        post_r7 = known + score;
    }
};


class Room {
public:
    std::array<Team*, 4> teams {};
    int round_num {};

    Room() = default;
    Room(std::array<Team*, 4> tms, int rn): teams {tms}, round_num {rn} {};
};

class R7Room : public Room {
public:
    std::set<R8Room*> later_rooms {}; // Vect because length mightn't be 4

    R7Room(std::array<Team*, 4> tms, int rn) : Room(tms, rn) {
        for (Team* team : teams) team->r7_room = this;
    };
};

class R8Room : public Room {
public:
    std::array<int, 4> post_r7s {};
    std::vector<int> pullups {};

    R8Room(std::array<Team*, 4> tms, int rn) : Room(tms, rn) {
        for (Team* team : teams) team->r8_room = this;
    };
};


void set_order(R7Room& r7_room, std::array<int, 4> order) {
    // Give the scores to the teams
    for (int i {0}; i < 4; i++) {
        r7_room.teams[i]->set_score(order[i]);
    }
    // Update data for the relevant r8 rooms
    for (R8Room* r8_room : r7_room.later_rooms) {
        // Fix up the room's post_r7 score list
        for (int i {0}; i < 4; i++) {
            r8_room->post_r7s[i] = r8_room->teams[i]->post_r7;
        }
        // Fix up the room's pullup list
        r8_room->pullups.clear();
        int max_val = *std::max_element(
            r8_room->post_r7s.begin(),
            r8_room->post_r7s.end()
        );
        for (int post_r7 : r8_room->post_r7s) {
            if (post_r7 < max_val) {
                r8_room->pullups.push_back(post_r7);
            }
        }
    }
}


void update_globs(R7Room& r7_room, bool subtract_mode=false) {
    int increment {(subtract_mode) ? -1 : 1};
    // 1. Update pullup loss
    for (R8Room* r8_room : r7_room.later_rooms) {
        for (int curr_pullup : r8_room->pullups) {
            upd[curr_pullup] += increment;
        }
    }
    // 2. Update sandwich loss
    for (Team* team : r7_room.teams) {
        usd[team->post_r7] += increment;
    }
}


void set_order_update_glob(R7Room& r7_room, std::array<int, 4> order) {
    // Subtract old contributions, update order, add new contributions
    update_globs(r7_room, true);
    set_order(r7_room, order);
    update_globs(r7_room, false);
}


int get_r8_room_sandwich_loss(R8Room& r8_room) {
    /*
    Returns sandwich loss for the room
    Which is the sum of entries in usd with indices strictly between
    the min and max team scores in the room
    Offset is to take away intra-room sandwiches
    */
    auto mm = std::minmax_element(
        r8_room.post_r7s.begin(),
        r8_room.post_r7s.end()
    );
    int filling_loss {0};
    for (int i {*mm.first + 1}; i < *mm.second; i++) filling_loss += usd[i];
    int offset = std::count_if(
        r8_room.pullups.begin(),
        r8_room.pullups.end(),
        [mm](int val) { return val > *mm.first; }
    );
    return filling_loss - offset;
}


int get_r7_room_loss(R7Room& r7_room) {
    int sandwich_loss {0};
    for (R8Room* r8_room : r7_room.later_rooms) {
        sandwich_loss += get_r8_room_sandwich_loss(*r8_room);
    }
    int pullup_loss {0};
    for (int pullup : upd) if (pullup > 3) pullup_loss += pullup - 3;
    return sandwich_loss + pullup_loss;
}


std::map<std::string, Team> get_teams(std::string directory) {
    /* Returns a map with key being team name
    and value being a Team object */
    std::map<std::string, Team> team_dict;
    std::ifstream file(directory + "/standings.csv");
    std::string line, name;
    int known;

    std::getline(file, line); // Skip header
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::getline(ss, name, ',');
        ss >> known;
        Team new_team {name, known};
        team_dict[name] = new_team;
    }
    return team_dict;
}


template<typename RoomType>
std::vector<RoomType*> get_round_rooms(
    std::string directory,
    std::map<std::string, Team>& teams,
    int round
) {
    std::vector <RoomType*> round_rooms;
    std::ifstream file(directory + "/r" + std::to_string(round) + "_draw.csv");
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::vector<Team*> temp_team_pointers;
        std::stringstream ss(line);
        std::string team_name;
        // Collect pointers to the teams in the new room
        while (std::getline(ss, team_name, ',')) {
            temp_team_pointers.push_back(&teams[team_name]);
        }
        // Make new room
        std::array<Team*, 4> team_pointers {};
        for (int i {0}; i < 4; i++) team_pointers[i] = temp_team_pointers[i];
        round_rooms.push_back(new RoomType {team_pointers, round});
    }
    return round_rooms;
}


void initialise(
    std::string& dir,
    std::map<std::string, Team>& teams,
    std::vector<R7Room*>& r7_rooms,
    std::vector<R8Room*>& r8_rooms
) {
    // Get the relevant objects initialised
    teams = get_teams(dir);
    // Pass teams by reference to get_round_rooms
    r7_rooms = get_round_rooms<R7Room>(dir, teams, 7);
    r8_rooms = get_round_rooms<R8Room>(dir, teams, 8);
    // Link the r7 rooms to the appropriate r8 rooms
    for (R7Room* r7_room : r7_rooms) {
        for (Team* team : r7_room->teams) {
            if (team->r8_room) {
                r7_room->later_rooms.insert(team->r8_room);
            }
        }
    }
}


void reset_results(
    std::map<std::string, Team>& teams,
    std::vector<R7Room*>& r7_rooms,
    std::vector<R8Room*>& r8_rooms
) {
    // Teams that miss r7 get 0
    for (auto [key, team] : teams) if (not team.r7_room) team.r7_est = 0;

    // Assign a random result per room
    for (R7Room* r7_room : r7_rooms) set_order(*r7_room, orders[rand() % 24]);

    // Reset globals
    std::fill(upd.begin(), upd.end(), 0);
    std::fill(usd.begin(), usd.end(), 0);
    for (R8Room* r8_room : r8_rooms) {
        for (int pullup : r8_room->pullups) upd[pullup] += 1;
    }
    for (auto const& [key, team] : teams) {
        usd[team.post_r7] += 1;
    }
}


int get_global_pullup_loss() {
    int pullup_loss {0};
    for (int pullup : upd) if (pullup > 3) pullup_loss += pullup - 3;
    return pullup_loss;
}


int get_global_sandwich_loss(std::vector<R8Room*>& r8_rooms) {
    int sandwich_loss {0};
    for (R8Room* r8_room : r8_rooms) {
        sandwich_loss += get_r8_room_sandwich_loss(*r8_room);
    }
    return sandwich_loss;
}


int get_global_loss (std::vector<R8Room*>& r8_rooms) {
    return get_global_pullup_loss() + get_global_sandwich_loss(r8_rooms);
}


void optimise_single_room(R7Room& r7_room) {
    int best_score {10000000};
    std::array<int, 4> best_order {0, 0, 0, 0};
    unsigned sd = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(orders.begin(), orders.end(), std::default_random_engine(sd));
    for (std::array<int, 4> order : orders) {
        set_order_update_glob(r7_room, order);
        int loss {get_r7_room_loss(r7_room)};
        if (loss < best_score) {
            best_score = loss;
            best_order = order;
        }
    }
    set_order_update_glob(r7_room, best_order);
}


void export_prediction(
    std::map<std::string, Team>& teams, std::string filename
) {
    int num_completed_sims {0};
    bool file_exists {false};

    // Check if file exists and count lines
    std::ifstream check_file(filename);
    if (check_file.is_open()) {
        file_exists = true;
        std::string line;
        while (std::getline(check_file, line)) num_completed_sims++;
        num_completed_sims -= 1;
        check_file.close();
    }

    // Write header if it needs to be written
    std::ofstream outfile(filename, std::ios::app);
    if (!file_exists) {
        outfile << "sim_num";
        for (const auto& pair : teams) outfile << "," << pair.first;
    }

    // Write in data for current sim
    outfile << "\n" << num_completed_sims;
    for (const auto& pair : teams) outfile << "," << pair.second.r7_est;
    outfile.close();
}


void print_predictions_r7(std::vector<R7Room*>& r7_rooms) {
    for (R7Room* r7_room : r7_rooms) {
        std::cout << "New room\n";
        for (Team* team : r7_room->teams) {
            std::cout << "\t" << team->r7_est << "\t" << team->name << "\n";
        }
    }
}


void print_predictions_r8(std::vector<R8Room*>& r8_rooms) {
    for (R8Room* r8_room : r8_rooms) {
        std::cout << "New room\n";
        for (Team* team : r8_room->teams) {
            std::cout << "\t" << team->post_r7 << "\t" << team->name << "\n";
        }
    }
}


void single_full_run(
    std::map<std::string, Team>& teams,
    std::vector<R7Room*>& r7_rooms,
    std::vector<R8Room*>& r8_rooms,
    int iterations,
    std::string filename,
    int threshold=0
) {
    int global_loss {};
    reset_results(teams, r7_rooms, r8_rooms);
    for (int i {0}; i < iterations; i++) {
        for (R7Room* r7_room : r7_rooms) optimise_single_room(*r7_room);
        global_loss = get_global_loss(r8_rooms);
        //std::cout << "Iter " << i + 1 << " loss: " << global_loss << "\n";
        if (global_loss <= threshold) {
            std::cout << "\tSUCCESS - exporting to file\n";
            export_prediction(teams, filename);
            return;
        }
    }
    std::cout << "\tFAILURE - starting again, loss " << global_loss << "\n";
    // print_predictions_r8(r8_rooms);
}


void multi_runs(
    std::map<std::string, Team>& teams,
    std::vector<R7Room*>& r7_rooms,
    std::vector<R8Room*>& r8_rooms,
    int iterations,
    int runs,
    std::string filename,
    int threshold=0
) {
    for (int i {0}; i < runs; i++) {
        std::cout << "STARTING iteration " << i + 1 << ":\t";
        single_full_run(
            teams, r7_rooms, r8_rooms, iterations, filename, threshold
        );
        // print_predictions_r7(r7_rooms);
        // print_predictions_r8(r8_rooms);
    }
}


int main(int argc, char* argv[]) {
    srand(time(nullptr));
    // Configurable bits
    std::string directory {argv[1]}; // Where the files are
    std::string filename {argv[2]}; // Where to put the output
    // std::string directory {"old_data/2022"};
    // std::string filename {"hastytab_output_nobread.csv"};
    int iterations {50}; // How many optimisation rounds it does
    int runs {1000}; // How many times it restarts from the top

    // Now run the program
    std::map<std::string, Team> teams;
    std::vector<R7Room*> r7_rooms;
    std::vector<R8Room*> r8_rooms;
    initialise(directory, teams, r7_rooms, r8_rooms);
    multi_runs(
        teams, r7_rooms, r8_rooms, iterations, runs, filename, 0
    );
    return 0;
}







// placeholder cheers

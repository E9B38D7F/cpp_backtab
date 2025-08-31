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
class R9Room;

// global variables
std::array<std::array<int, 4>, 24> orders {{
    {0,1,2,3}, {0,1,3,2}, {0,2,1,3}, {0,2,3,1}, {0,3,1,2}, {0,3,2,1},
    {1,0,2,3}, {1,0,3,2}, {1,2,0,3}, {1,2,3,0}, {1,3,0,2}, {1,3,2,0},
    {2,0,1,3}, {2,0,3,1}, {2,1,0,3}, {2,1,3,0}, {2,3,0,1}, {2,3,1,0},
    {3,0,1,2}, {3,0,2,1}, {3,1,0,2}, {3,1,2,0}, {3,2,0,1}, {3,2,1,0}
}};
std::array<int, 28> upd_8 {}; // Universal Pullup Dict
std::array<int, 28> upd_9 {};
std::array<int, 28> usd_8 {}; // Universal Sandwich Dict
std::array<int, 28> usd_9 {};


class Team {
public:
    std::string name {};
    int known {};
    int r7_est {};
    int post_r7 {};
    int r8_est {};
    int post_r8 {};
    std::vector<int> poss_r7 {};
    R7Room* r7_room {nullptr};
    R8Room* r8_room {nullptr};
    R9Room* r9_room {nullptr};

    Team() = default;
    Team(std::string nm, int kn): name {nm}, known {kn} {
        this->set_score_r7(0); // Necessary for teams that skip r7
        this->set_score_r8(0);
    };

    void set_score_r7(int score) {
        r7_est = score;
        post_r7 = known + score;
        post_r8 = known + score + r8_est;
    }

    void set_score_r8(int score) {
        r8_est = score;
        post_r8 = known + r7_est + score;
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
    std::set<R8Room*> later_r8_rooms {}; // Vect because length mightn't be 4
    std::set<R9Room*> later_r9_rooms {};
    std::vector<std::array<int, 4>> poss_orders {}; // Possible r7 results
        // which are set by looking at output of r7 tab

    R7Room(std::array<Team*, 4> tms, int rn) : Room(tms, rn) {
        for (Team* team : teams) team->r7_room = this;
    };
};

class R8Room : public Room {
public:
    std::array<int, 4> post_r7s {};
    std::vector<int> pullups {};
    std::set<R9Room*> later_r9_rooms {};

    R8Room(std::array<Team*, 4> tms, int rn) : Room(tms, rn) {
        for (Team* team : teams) team->r8_room = this;
    };
};

class R9Room : public Room {
public:
    std::array<int, 4> post_r8s {};
    std::vector<int> pullups {};

    R9Room(std::array<Team*, 4> tms, int rn) : Room(tms, rn) {
        for (Team* team : teams) team->r9_room = this;
    };
};


void set_order_r7(R7Room& r7_room, std::array<int, 4> order) {
    // Give the scores to the teams
    for (int i {0}; i < 4; i++) {
        r7_room.teams[i]->set_score_r7(order[i]);
    }
    // Update data for the relevant r8 rooms
    for (R8Room* r8_room : r7_room.later_r8_rooms) {
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
    // Now do the same for r9 rooms (copypasted)
    for (R9Room* r9_room : r7_room.later_r9_rooms) {
        // Fix up the room's post_r7 score list
        for (int i {0}; i < 4; i++) {
            r9_room->post_r8s[i] = r9_room->teams[i]->post_r8;
        }
        // Fix up the room's pullup list
        r9_room->pullups.clear();
        int max_val = *std::max_element(
            r9_room->post_r8s.begin(),
            r9_room->post_r8s.end()
        );
        for (int post_r8 : r9_room->post_r8s) {
            if (post_r8 < max_val) {
                r9_room->pullups.push_back(post_r8);
            }
        }
    }
}

void set_order_r8(R8Room& r8_room, std::array<int, 4> order) {
    // This code might be starting to look familiar...
    // Give the scores to the teams
    for (int i {0}; i < 4; i++) {
        r8_room.teams[i]->set_score_r8(order[i]);
    }
    // Now do the same for r9 rooms
    for (R9Room* r9_room : r8_room.later_r9_rooms) {
        // Fix up the room's post_r7 score list
        for (int i {0}; i < 4; i++) {
            r9_room->post_r8s[i] = r9_room->teams[i]->post_r8;
        }
        // Fix up the room's pullup list
        r9_room->pullups.clear();
        int max_val = *std::max_element(
            r9_room->post_r8s.begin(),
            r9_room->post_r8s.end()
        );
        for (int post_r8 : r9_room->post_r8s) {
            if (post_r8 < max_val) {
                r9_room->pullups.push_back(post_r8);
            }
        }
    }
}

void update_globs_r7(R7Room& r7_room, bool subtract_mode=false) {
    int increment {(subtract_mode) ? -1 : 1};
    // 1. Update pullup loss
    for (R8Room* r8_room : r7_room.later_r8_rooms) {
        for (int curr_pullup : r8_room->pullups) {
            upd_8[curr_pullup] += increment;
        }
    }
    for (R9Room* r9_room : r7_room.later_r9_rooms) {
        for (int curr_pullup : r9_room->pullups) {
            upd_9[curr_pullup] += increment;
        }
    }
    // 2. Update sandwich loss
    for (Team* team : r7_room.teams) {
        usd_8[team->post_r7] += increment;
        usd_9[team->post_r8] += increment;
    }
}

void update_globs_r8(R8Room& r8_room, bool subtract_mode=false) {
    int increment {(subtract_mode) ? -1 : 1};
    // 1. Update pullup loss
    for (R9Room* r9_room : r8_room.later_r9_rooms) {
        for (int curr_pullup : r9_room->pullups) {
            upd_9[curr_pullup] += increment;
        }
    }
    // 2. Update sandwich loss
    for (Team* team : r8_room.teams) {
        usd_9[team->post_r8] += increment;
    }
}

void set_order_update_glob_r7(R7Room& r7_room, std::array<int, 4> order) {
    // Subtract old contributions, update order, add new contributions
    update_globs_r7(r7_room, true);
    set_order_r7(r7_room, order);
    update_globs_r7(r7_room, false);
}

void set_order_update_glob_r8(R8Room& r8_room, std::array<int, 4> order) {
    // Subtract old contributions, update order, add new contributions
    update_globs_r8(r8_room, true);
    set_order_r8(r8_room, order);
    update_globs_r8(r8_room, false);
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
    for (int i {*mm.first + 1}; i < *mm.second; i++) filling_loss += usd_8[i];
    int offset = std::count_if(
        r8_room.pullups.begin(),
        r8_room.pullups.end(),
        [mm](int val) { return val > *mm.first; }
    );
    return filling_loss - offset;
}

int get_r9_room_sandwich_loss(R9Room& r9_room) {
    // Ditto
    auto mm = std::minmax_element(
        r9_room.post_r8s.begin(),
        r9_room.post_r8s.end()
    );
    int filling_loss {0};
    for (int i {*mm.first + 1}; i < *mm.second; i++) filling_loss += usd_9[i];
    int offset = std::count_if(
        r9_room.pullups.begin(),
        r9_room.pullups.end(),
        [mm](int val) { return val > *mm.first; }
    );
    return filling_loss - offset;
}

int get_r7_room_loss(R7Room& r7_room) {
    // Look forward to both r8 AND r9 rooms
    int sandwich_loss {0};
    for (R8Room* r8_room : r7_room.later_r8_rooms) {
        sandwich_loss += get_r8_room_sandwich_loss(*r8_room);
    }
    for (R9Room* r9_room : r7_room.later_r9_rooms) {
        sandwich_loss += get_r9_room_sandwich_loss(*r9_room);
    }
    int pullup_loss {0};
    for (int pullup : upd_8) if (pullup > 3) pullup_loss += pullup - 3;
    for (int pullup : upd_9) if (pullup > 3) pullup_loss += pullup - 3;
    return sandwich_loss + pullup_loss;
}

int get_r8_room_loss(R8Room& r8_room) {
    // Can only look forward to r9 rooms
    int sandwich_loss {0};
    for (R9Room* r9_room : r8_room.later_r9_rooms) {
        sandwich_loss += get_r9_room_sandwich_loss(*r9_room);
    }
    int pullup_loss {0};
    for (int pullup : upd_9) if (pullup > 3) pullup_loss += pullup - 3;
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
    std::string& r7_filename,
    std::map<std::string, Team>& teams,
    std::vector<R7Room*>& r7_rooms,
    std::vector<R8Room*>& r8_rooms,
    std::vector<R9Room*>& r9_rooms
) {
    // Get the relevant objects initialised
    teams = get_teams(dir);
    // Pass teams by reference to get_round_rooms
    r7_rooms = get_round_rooms<R7Room>(dir, teams, 7);
    r8_rooms = get_round_rooms<R8Room>(dir, teams, 8);
    r9_rooms = get_round_rooms<R9Room>(dir, teams, 9);
    // Link rooms to each other:
    for (R8Room* r8_room : r8_rooms) { // First r8 to r9
        for (Team* team : r8_room->teams) {
            if (team->r9_room) r8_room->later_r9_rooms.insert(team->r9_room);
        }
    }
    for (R7Room* r7_room : r7_rooms) {
        for (Team* team : r7_room->teams) { // Then r7 to r8
            if (team->r8_room) r7_room->later_r8_rooms.insert(team->r8_room);
        }
        for (R8Room* r8_room : r7_room->later_r8_rooms) { // And r7 to r9
            for (R9Room* r9_room : r8_room->later_r9_rooms) {
                r7_room->later_r9_rooms.insert(r9_room);
            }
        }
    }
    // Import r7 backtab output to save on effort
    // 1. Read in data and put into the relevant Team objects
    std::vector<std::string> column_heads;
    std::ifstream file(r7_filename);
    std::string line;
    std::string col_name;
    std::getline(file, line);
    std::stringstream ss(line);
    while (std::getline(ss, col_name, ',')) {
        column_heads.push_back(col_name);
    }
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int col_num {0};
        std::string score_str {};
        while (std::getline(ss, score_str, ',')) {
            if (col_num == 0) { // i.e., on "sim_num" column
                col_num++;
                continue;
            }
            int est_score {std::stoi(score_str)};
            Team& rel_team = teams.at(column_heads[col_num]);
            bool score_already_got {false};
            for (int score : rel_team.poss_r7) {
                if (score == est_score) {
                    score_already_got = true;
                    break;
                }
            }
            if (!score_already_got) {
                rel_team.poss_r7.push_back(est_score);
            }
            col_num++;
        }
    }
    // 2. For each r7 room, get possible orders from relevant Teams
    for (R7Room* r7_room : r7_rooms) {
        for (int a : r7_room->teams[0]->poss_r7) {
            for (int b : r7_room->teams[1]->poss_r7) {
                for (int c : r7_room->teams[2]->poss_r7) {
                    for (int d : r7_room->teams[3]->poss_r7) {
                        if ( // Check the order is valid
                            (a == b) | (a == c) | (a == d)
                            | (b == c) | (b == d) | (c == d)
                        ) continue;
                        std::array<int, 4> new_order {a, b, c, d};
                        r7_room->poss_orders.push_back(new_order);
                    }
                }
            }
        }
    }
}


void reset_results(
    std::map<std::string, Team>& teams,
    std::vector<R7Room*>& r7_rooms,
    std::vector<R8Room*>& r8_rooms,
    std::vector<R9Room*>& r9_rooms
) {
    // Teams that miss r7 get 0
    for (auto [key, team] : teams) if (not team.r7_room) team.r7_est = 0;

    // Assign a random result per room
    for (R7Room* r7_room : r7_rooms) {
        set_order_r7(*r7_room, orders[rand() % 24]);
    }
    for (R8Room* r8_room : r8_rooms) {
        set_order_r8(*r8_room, orders[rand() % 24]);
    }

    // Reset globals
    std::fill(upd_8.begin(), upd_8.end(), 0);
    std::fill(usd_8.begin(), usd_8.end(), 0);
    std::fill(upd_9.begin(), upd_9.end(), 0);
    std::fill(usd_9.begin(), usd_9.end(), 0);
    for (R8Room* r8_room : r8_rooms) {
        for (int pullup : r8_room->pullups) upd_8[pullup] += 1;
    }
    for (R9Room* r9_room : r9_rooms) {
        for (int pullup : r9_room->pullups) upd_9[pullup] += 1;
    }
    for (auto const& [key, team] : teams) {
        usd_8[team.post_r7] += 1;
        usd_9[team.post_r8] += 1;
    }
}


int get_global_pullup_loss() {
    int pullup_loss {0};
    for (int pullup : upd_8) if (pullup > 3) pullup_loss += pullup - 3;
    // std::cout << pullup_loss << " ";
    for (int pullup : upd_9) if (pullup > 3) pullup_loss += pullup - 3;
    // std::cout << pullup_loss << "\n";
    return pullup_loss;
}


int get_global_sandwich_loss(
    std::vector<R8Room*>& r8_rooms,
    std::vector<R9Room*>& r9_rooms
) {
    int sandwich_loss {0};
    for (R8Room* r8_room : r8_rooms) {
        sandwich_loss += get_r8_room_sandwich_loss(*r8_room);
    }
    // std::cout << sandwich_loss << " ";
    for (R9Room* r9_room : r9_rooms) {
        sandwich_loss += get_r9_room_sandwich_loss(*r9_room);
    }
    // std::cout << sandwich_loss << "\n";
    return sandwich_loss;
}


int get_global_loss(
    std::vector<R8Room*>& r8_rooms,
    std::vector<R9Room*>& r9_rooms
) {
    int gpl {get_global_pullup_loss()};
    int gsl {get_global_sandwich_loss(r8_rooms, r9_rooms)};
    return gpl + gsl;
}


void optimise_single_room_r7(R7Room& r7_room) {
    int best_score {10000000};
    std::array<int, 4> best_order {0, 0, 0, 0};
    unsigned sd = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(
        r7_room.poss_orders.begin(),
        r7_room.poss_orders.end(),
        std::default_random_engine(sd)
    );
    for (std::array<int, 4> order : r7_room.poss_orders) {
        set_order_update_glob_r7(r7_room, order);
        int loss {get_r7_room_loss(r7_room)};
        if (loss < best_score) {
            best_score = loss;
            best_order = order;
        }
    }
    set_order_update_glob_r7(r7_room, best_order);
}


void optimise_single_room_r8(R8Room& r8_room) {
    int best_score {10000000};
    std::array<int, 4> best_order {0, 0, 0, 0};
    unsigned sd = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(orders.begin(), orders.end(), std::default_random_engine(sd));
    for (std::array<int, 4> order : orders) {
        set_order_update_glob_r8(r8_room, order);
        int loss {get_r8_room_loss(r8_room)};
        if (loss < best_score) {
            best_score = loss;
            best_order = order;
        }
    }
    set_order_update_glob_r8(r8_room, best_order);
}


void print_predictions_r8(std::vector<R8Room*>& r8_rooms) {
    for (R8Room* r8_room : r8_rooms) {
        std::cout << "New r8 room\n";
        for (Team* team : r8_room->teams) {
            std::cout << "\t" << team->post_r7 << "\t" << team->name << "\n";
        }
    }
}


void print_predictions_r9(std::vector<R9Room*>& r9_rooms) {
    for (R9Room* r9_room : r9_rooms) {
        std::cout << "New r9 room\n";
        for (Team* team : r9_room->teams) {
            std::cout << "\t" << team->post_r8 << "\t" << team->name << "\n";
        }
    }
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
        for (const auto& pair : teams) {
            outfile << "," << pair.first << "_r7";
            outfile << "," << pair.first << "_r8";
        }
    }

    // Write in data for current sim
    outfile << "\n" << num_completed_sims;
    for (const auto& pair : teams) {
        outfile << "," << pair.second.r7_est;
        outfile << "," << pair.second.r8_est;
    }
    outfile.close();
}


void single_full_run(
    std::map<std::string, Team>& teams,
    std::vector<R7Room*>& r7_rooms,
    std::vector<R8Room*>& r8_rooms,
    std::vector<R9Room*>& r9_rooms,
    int r7_iterations,
    int r8_iterations,
    std::string filename,
    int threshold=0
) {
    int global_loss {};
    reset_results(teams, r7_rooms, r8_rooms, r9_rooms);
    for (int i {0}; i < r8_iterations; i++) {
        if (i < r7_iterations){
            for (R7Room* r7_room : r7_rooms) optimise_single_room_r7(*r7_room);
        }
        for (R8Room* r8_room : r8_rooms) optimise_single_room_r8(*r8_room);
        global_loss = get_global_loss(r8_rooms, r9_rooms);
        if (global_loss <= threshold) {
            std::cout << "\tSUCCESS - exporting; loss " << global_loss << "\n";
            export_prediction(teams, filename);
            return;
        }
    }
    std::cout << "\tFAILURE - starting again, loss " << global_loss << "\n";
}


void multi_runs(
    std::map<std::string, Team>& teams,
    std::vector<R7Room*>& r7_rooms,
    std::vector<R8Room*>& r8_rooms,
    std::vector<R9Room*>& r9_rooms,
    int r7_iterations,
    int r8_iterations,
    int runs,
    std::string filename,
    int threshold=0
) {
    for (int i {0}; i < runs; i++) {
        std::cout << "STARTING iteration " << i + 1 << ":\t";
        single_full_run(
            teams, r7_rooms, r8_rooms, r9_rooms,
            r7_iterations, r8_iterations, filename, threshold
        );
    }
}


int main() {
    srand(time(nullptr));
    // Configurable bits
    std::string directory {"old_data/2022"};
    std::string r7_filename {"hastytab_output_nobread.csv"};
    int r7_iterations {20};
    int r8_iterations {50};
    int runs {100};
    std::string filename {"hastytab_output_nobread_r8.csv"};

    // Now run the program
    std::map<std::string, Team> teams;
    std::vector<R7Room*> r7_rooms;
    std::vector<R8Room*> r8_rooms;
    std::vector<R9Room*> r9_rooms;
    initialise(directory, r7_filename, teams, r7_rooms, r8_rooms, r9_rooms);
    multi_runs(
        teams, r7_rooms, r8_rooms, r9_rooms,
        r7_iterations, r8_iterations, runs, filename, 0
    );
    // print_predictions_r9(r9_rooms);
    return 0;
}







// placeholder cheers

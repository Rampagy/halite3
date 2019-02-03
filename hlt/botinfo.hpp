#ifndef BOT_INFO_H
#define BOT_INFO_H

#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "hlt/astar.hpp"

#include <random>
#include <ctime>
#include <unordered_map>
#include <map>
#include <cmath>
#include <queue>
#include <algorithm>

using namespace std;
using namespace hlt;

typedef pair<float, shared_ptr<Ship>> impatience_T;
typedef pair<float, Position> desirability_T;
typedef pair<int, Position> closest_drop_T;
typedef pair<float, Direction> direction_cost_T;


auto value_selector = [](auto pair){return pair.second;};


struct BotInfo {
    const int my_id;
    const int num_players;
    const int width;
    const int height;
    const int capita_per_person;

    // index order is playerid, second is height, third is width
    vector<vector<vector<closest_drop_T>>> dropoff_dist;
    // index order is playerid, second is height, third is width
    vector<vector<vector<EntityId>>> ship_map;
    // index order is height, second is width
    vector<vector<Halite>> halite_map;
    // index is height, second is width
    vector<vector<EntityId>> ships_next_position;
    // index is height, second is width
    vector<vector<bool>> inspired_map;

    /*     Start non-constructor variables     */

    // track the farthest ships distance
    int farthest_ship_dist = 0;

    // track the current turn
    int turn_number = 0;

    // starting halite on the map
    Halite starting_halite;

    // current halite on the map
    Halite current_halite;

    // track each players current halite
    unordered_map<EntityId, Halite> players_halite;

    // keep track of players previous dropoffs
    unordered_map<PlayerId, int> prev_drops_count;

    // keep track of every players ship
    unordered_map<PlayerId, vector<shared_ptr<Ship>>> players_ships;

    // track ships impatience
    priority_queue<impatience_T> ship_queue;

    // how much halite I have in the bank
    int halite_in_bank = 0;

    // if we are saving for a dropoff
    bool saving_for_dropoff = false;

    // my shipyard position
    shared_ptr<Shipyard> shipyard;

    // vector of unmoveable ships
    queue<EntityId> unmovable_ships;

    // set for ships headed to dropoff
    unordered_set<EntityId> headed_to_dropoff;

    // if we are in end game or not
    bool end_game = false;

    // track current targets
    unordered_set<Position> currently_targeted;

    // track how many times a ship has wanted to move, but hasn't
    unordered_map<EntityId, int> ship_move_counter;

    // drop off counter
    int dropoff_counter = 0;

    /*     Start public functions/methods     */

    BotInfo(int my_id, int num_players, int width, int height) :
        my_id(my_id),
        num_players(num_players),
        width(width),
        height(height),
        capita_per_person((height * width) / num_players),
        dropoff_dist(num_players, vector<vector<closest_drop_T>>(height, vector<closest_drop_T>(width, make_pair(0, Position(0, 0))))),
        ship_map(num_players, vector<vector<EntityId>>(height, vector<EntityId>(width, -1))),
        halite_map(height, vector<Halite>(width, 0)),
        ships_next_position(height, vector<EntityId>(width, -1)),
        inspired_map(height, vector<bool>(width, false))
    {}


    void update_per_turn(void);

    void update_dropoff_map(const PlayerId& player_id,
            const unordered_map<EntityId, shared_ptr<Dropoff>>& dropoffs,
            const shared_ptr<Shipyard>& shipyard);

    void update_ship_map(const PlayerId& player_id,
            const unordered_map<EntityId, shared_ptr<Ship>>& ships,
            const Halite& player_halite);

    void build_inspired_map(const unique_ptr<GameMap>& game_map);

    void build_new_ship(vector<Command>& command_queue);

    void make_move(vector<Command>& command_queue,
            unordered_map<EntityId, std::shared_ptr<Ship>>& ships);

    void build_dropoff(vector<Command>& command_queue,
            unordered_map<EntityId, shared_ptr<Ship>>& ships);

    /*     Start private functions/methods     */

    float _euclidean_distance(const Position& source, const Position& target);
    void _retrace_path(const Position& start, const Position& end, Halite& path_cost,
            int& path_dist);
    void _get_next_ship(const Position& destination, bool& find_move,
            bool& ship_must_move, shared_ptr<Ship>& move_ship,
            unordered_map<EntityId, shared_ptr<Ship>>& my_ships);
    Direction _navigate(const Position& start, const Position& end,
            Halite& path_cost, int& path_dist, const EntityId& ship_id,
            const bool& safe_mode = false);
    Halite _get_stay_benefit(const Position& pos, const int& stay_time);
    Position _find_target(const shared_ptr<Ship>& ship);
    int _calculate_distance(const Position& source, const Position& target);
    Position _normalize(const Position& position);
};

#endif

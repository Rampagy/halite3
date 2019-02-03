#include "botinfo.hpp"

/*         Start public methods/functions         */

void BotInfo::update_dropoff_map(const PlayerId& player_id,
        const unordered_map<EntityId, shared_ptr<Dropoff>>& dropoffs,
        const shared_ptr<Shipyard>& shipyard)
{
    unordered_map<EntityId, Position> drop_with_shipyard;
    for (const auto& drop_iter : dropoffs)
    {
        drop_with_shipyard[drop_iter.first] = drop_iter.second->position;
    }
    drop_with_shipyard[999] = shipyard->position;
    if (player_id == this->my_id)
    {
        this->shipyard = shipyard;
    }


    if ((int)drop_with_shipyard.size() != this->prev_drops_count[player_id])
    {
        this->prev_drops_count[player_id] = drop_with_shipyard.size();
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                int dist = 9999;
                Position drop = Position(x, y);
                for (const auto& drop_iter : drop_with_shipyard)
                {
                    int temp = _calculate_distance(Position(x, y), drop_iter.second);
                    if (temp < dist)
                    {
                        dist = temp;
                        drop =  drop_iter.second;
                    }
                }

                this->dropoff_dist[player_id][y][x] = make_pair(dist, drop);
            }
        }
    }
    return;
}


void BotInfo::update_ship_map(const PlayerId& player_id,
        const unordered_map<EntityId, shared_ptr<Ship>>& ships,
        const Halite& player_halite)
{
    // ships will move positions almost every turn so update this every time
    for (int y = 0; y < this->height; y++)
    {
        fill(this->ship_map[player_id][y].begin(), this->ship_map[player_id][y].end(), -1);
        if (player_id == this->my_id)
        {
            fill(this->ships_next_position[y].begin(), this->ships_next_position[y].end(), -1);
        }
    }

    this->players_halite[player_id] = player_halite;
    if (player_id == this->my_id)
    {
        halite_in_bank = player_halite;
    }

    vector<shared_ptr<Ship>> ship_vec(ships.size());
    transform(ships.begin(), ships.end(), ship_vec.begin(), value_selector);
    this->players_ships[player_id] = ship_vec;

    this->farthest_ship_dist = 0;
    for (const auto& ship_iter : ships)
    {
        this->players_halite[player_id] += ship_iter.second->halite;
        this->ship_map[player_id][ship_iter.second->position.y][ship_iter.second->position.x] = ship_iter.first;

        if (player_id == my_id)
        {
            if (this->ship_move_counter.find(ship_iter.first) == this->ship_move_counter.end())
            {
                this->ship_move_counter[ship_iter.first] = 0;
            }

            int closest_drop_dist = this->dropoff_dist[this->my_id][ship_iter.second->position.y][ship_iter.second->position.x].first;

            // track the farthest ships distance from the dropoff
            if (closest_drop_dist > this->farthest_ship_dist)
            {
                this->farthest_ship_dist = closest_drop_dist;
            }

            if (ship_iter.second->halite*constants::MOVE_COST_RATIO >=
                    this->halite_map[ship_iter.second->position.y][ship_iter.second->position.x])
            {
                // update ships impatience
                float score = ship_iter.second->halite +
                        this->dropoff_dist[this->my_id][ship_iter.second->position.y][ship_iter.second->position.x].first +
                        this->halite_map[ship_iter.second->position.y][ship_iter.second->position.x];
                this->ship_queue.push(make_pair(score, ship_iter.second));
            }
            else
            {
                // add to unmoveable
                this->unmovable_ships.push(ship_iter.first);
            }
        }
    }
    if (player_id == my_id)
    {
        if (!this->end_game)
        {
            this->end_game = ((constants::MAX_TURNS - this->farthest_ship_dist - 3) < this->turn_number);
        }
    }
    return;
}


void BotInfo::build_inspired_map(const unique_ptr<GameMap>& game_map)
{
    this->current_halite = 0;
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            Halite hal = game_map->at(Position(x, y))->halite;
            this->current_halite += hal;
            this->halite_map[y][x] = hal;

            int ship_count = 0;
            bool done = false;
            for (const auto& player_ships : this->players_ships)
            {
                if (player_ships.first != this->my_id)
                {
                    for (shared_ptr<Ship> ship : player_ships.second)
                    {
                        if (_calculate_distance(ship->position, Position(x, y)) <= constants::INSPIRATION_RADIUS)
                        {
                            ship_count++;
                        }

                        if (ship_count >= constants::INSPIRATION_SHIP_COUNT)
                        {
                            this->inspired_map[y][x] = true;
                            done = true;
                            break;
                        }
                    }
                }

                if (done)
                {
                    break;
                }
            }

            if (!done)
            {
                this->inspired_map[y][x] = false;
            }
        }
    }

    if (this->turn_number == 1)
    {
        this->starting_halite = this->current_halite;
    }
}


void BotInfo::build_new_ship(vector<Command>& command_queue)
{
    if (!this->saving_for_dropoff && (halite_in_bank >= constants::SHIP_COST) &&
        (this->ships_next_position[this->shipyard->position.y][this->shipyard->position.x] < 0) &&
        (constants::MAX_TURNS * 0.56 > this->turn_number) &&
        ((this->current_halite / (float)this->starting_halite) > 0.48))
    {
        command_queue.push_back(this->shipyard->spawn());
        this->ships_next_position[this->shipyard->position.y][this->shipyard->position.x] = 999;
        this->halite_in_bank -= constants::SHIP_COST;
    }
}


void BotInfo::make_move(vector<Command>& command_queue,
        unordered_map<EntityId, shared_ptr<Ship>>& ships)
{
    while (!this->unmovable_ships.empty())
    {
        shared_ptr<Ship> unmovable_ship = ships[this->unmovable_ships.front()];
        ships.erase(unmovable_ship->id);
        command_queue.push_back(unmovable_ship->move(Direction::STILL));
        this->ships_next_position[unmovable_ship->position.y][unmovable_ship->position.x] = unmovable_ship->id;
        this->unmovable_ships.pop();
    }

    shared_ptr<Ship> ship;
    bool ship_must_move = false;
    bool find_move = false;

    if (!this->ship_queue.empty())
    {
        while (ships.find(this->ship_queue.top().second->id) == ships.end())
        {
            this->ship_queue.pop();
        }

        ship = this->ship_queue.top().second;
        this->ship_queue.pop();
        find_move = true;
        ships.erase(ship->id);
    }

    while (find_move)
    {
        if ((this->dropoff_dist[this->my_id][ship->position.y][ship->position.x].first <= 0) &&
            (this->headed_to_dropoff.find(ship->id) != this->headed_to_dropoff.end()))
        {
            this->headed_to_dropoff.erase(ship->id);
        }

        Halite path_cost = 0;
        int path_dist = 0;
        Direction dir = Direction::STILL;

        if ((ship->halite > 900) ||
            (this->headed_to_dropoff.find(ship->id) != this->headed_to_dropoff.end()))
        {
            Position closest_drop = this->dropoff_dist[this->my_id][ship->position.y][ship->position.x].second;
            dir = this->_navigate(ship->position, closest_drop, path_cost, path_dist, ship->id, true);

            if (dir == Direction::STILL)
            {
                this->ship_move_counter[ship->id]++;
            }
            else
            {
                this->ship_move_counter[ship->id] = 0;
            }

            if (this->headed_to_dropoff.find(ship->id) == this->headed_to_dropoff.end())
            {
                headed_to_dropoff.insert(ship->id);
            }
        }
        else if (this->end_game)
        {
            Position closest_drop = this->dropoff_dist[this->my_id][ship->position.y][ship->position.x].second;
            dir = this->_navigate(ship->position, closest_drop, path_cost, path_dist, ship->id, true);

            if (dir == Direction::STILL)
            {
                this->ship_move_counter[ship->id]++;
            }
            else
            {
                this->ship_move_counter[ship->id] = 0;
            }
        }
        else
        {
            Position target = this->_find_target(ship);
            Direction tmp = this->_navigate(ship->position, target, path_cost, path_dist, ship->id);

            Halite move_benefit = this-> _get_stay_benefit(target, 1) - path_cost;
            Halite stay_benefit = this->_get_stay_benefit(ship->position, path_dist+1);

            if ((stay_benefit <= move_benefit) || (stay_benefit < 1) || ship_must_move)
            {
                this->currently_targeted.insert(target);
                dir = tmp;

                if (dir == Direction::STILL)
                {
                    this->ship_move_counter[ship->id]++;
                }
                else
                {
                    this->ship_move_counter[ship->id] = 0;
                }
            }
            else
            {
                dir = Direction::STILL;
                this->ship_move_counter[ship->id] = 0;
            }
        }

        if ((dir == Direction::STILL) && ship_must_move && !this->end_game)
        {
            for (Direction current_direction : ALL_CARDINALS)
            {
                Position check_pos = this->_normalize(
                            ship->position.directional_offset(current_direction));

                if (this->ships_next_position[check_pos.y][check_pos.x] < 0)
                {
                    dir = current_direction;
                    this->ship_move_counter[ship->id] = 0;
                    break;
                }
            }
        }

        command_queue.push_back(ship->move(dir));
        Position destination = this->_normalize(ship->position.directional_offset(dir));
        this->_get_next_ship(destination, find_move, ship_must_move, ship, ships);

        if (!this->end_game || (this->dropoff_dist[this->my_id][destination.y][destination.x].first > 0))
        {
            this->ships_next_position[destination.y][destination.x] = ship->id;
        }
    }
}


void BotInfo::update_per_turn()
{
    this->turn_number++;
    this->unmovable_ships = queue<EntityId>();
    this->ship_queue = priority_queue<impatience_T>();
    this->currently_targeted.clear();
}


void BotInfo::build_dropoff(vector<Command>& command_queue,
        unordered_map<EntityId, shared_ptr<Ship>>& ships)
{
    if (!ships.empty())
    {
        if (!this->saving_for_dropoff && (constants::MAX_TURNS - 50 > this->turn_number) &&
            (prev_drops_count[this->my_id] < this->capita_per_person/256) &&
            ((int)ships.size() > 18*prev_drops_count[this->my_id]) && (this->dropoff_counter > 75))
        {
            this->saving_for_dropoff = true;
            this->dropoff_counter = 0;
        }
        else
        {
            float ave_dist = 0;
            for (const auto& ship_iter : ships)
            {
                if (this->headed_to_dropoff.find(ship_iter.first) == this->headed_to_dropoff.end())
                {
                    ave_dist += this->dropoff_dist[this->my_id][ship_iter.second->position.y][ship_iter.second->position.x].first;
                }
            }

            // empty check in above if
            ave_dist /= (float)ships.size();
            if ((ave_dist >= 5.0) && (prev_drops_count[this->my_id] < this->capita_per_person/256) &&
                !this->saving_for_dropoff && (constants::MAX_TURNS - 50 > this->turn_number) &&
                ((int)ships.size() > 5) && (this->dropoff_counter > 75))
            {
                this->saving_for_dropoff = true;
                this->dropoff_counter = 0;
            }
            else
            {
                this->dropoff_counter++;
            }
        }

        if (this->saving_for_dropoff)
        {
            double max_score = -999999999;
            shared_ptr<Ship> best_ship = ships.begin()->second;
            for (const auto& ship_iter : ships)
            {
                bool structure_present = false;
                int closest_other_drop_dist = 999;
                int closest_me_drop_dist = 999;

                for (int i = 0; i < (int)this->dropoff_dist.size(); i++)
                {
                    int dist = dropoff_dist[i][ship_iter.second->position.y][ship_iter.second->position.x].first;

                    if (i == this->my_id)
                    {
                        closest_me_drop_dist = dist;
                    }
                    else
                    {
                        if (dist < closest_other_drop_dist)
                        {
                            closest_other_drop_dist = dist;
                        }
                    }

                    if (dist <= 0)
                    {
                        structure_present = true;
                        break;
                    }
                }

                if (!structure_present)
                {
                    shared_ptr<Ship> ship = ship_iter.second;

                    if ((closest_me_drop_dist > 7) && (closest_other_drop_dist > 5))
                    {
                        Halite hal = this->halite_map[ship->position.y][ship->position.x];
                        for (Position p : ship->position.get_surrounding_cardinals())
                        {
                            p = this->_normalize(p);
                            hal += this->halite_map[p.y][p.x];
                        }

                        // score each ship, choose biggest score
                        float score = hal/5 + 100*closest_me_drop_dist + 50*closest_other_drop_dist;
                        if (score > max_score)
                        {
                            max_score = score;
                            best_ship = ship;
                        }
                    }
                }
            }

            Halite cost = constants::DROPOFF_COST -
                this->halite_map[best_ship->position.y][best_ship->position.x] -
                best_ship->halite;

            if (this->halite_in_bank >= cost)
            {
                this->saving_for_dropoff = false;
                this->halite_in_bank -= cost;
                ships.erase(best_ship->id);
                command_queue.push_back(best_ship->make_dropoff());
            }
        }
    }
    return;
}

/*         Start private methods/functions         */

void BotInfo::_retrace_path(const Position& start, const Position& end, Halite& path_cost, int& path_dist)
{
    float epsilon = 0.86;
    Position current_pos = start;
    while (current_pos != end)
    {
        priority_queue<direction_cost_T, vector<direction_cost_T>, greater<direction_cost_T>> neighbors;

        for (const Direction dir : ALL_CARDINALS)
        {
            Position neighbor = this->_normalize(current_pos.directional_offset(dir));

            if (this->_calculate_distance(neighbor, end) < this->_calculate_distance(current_pos, end))
            {
                neighbors.push(make_pair(this->halite_map[neighbor.y][neighbor.x], dir));
            }
        }

        Direction dir = neighbors.top().second;
        current_pos = this->_normalize(current_pos.directional_offset(dir));
        path_dist += 1;
        path_cost += round(epsilon * this->halite_map[current_pos.y][current_pos.x] / (float)constants::MOVE_COST_RATIO);
        epsilon *= epsilon;
    }
}

void BotInfo::_get_next_ship(const Position& destination, bool& find_move,
                    bool& ship_must_move, shared_ptr<Ship>& move_ship,
                    unordered_map<EntityId, shared_ptr<Ship>>& my_ships)
{
    if ((destination == move_ship->position) && !my_ships.empty()) // stayed still
    {
        while (my_ships.find(this->ship_queue.top().second->id) == my_ships.end())
        {
            this->ship_queue.pop();
        }

        ship_must_move = false;
        move_ship = this->ship_queue.top().second;
        this->ship_queue.pop();
        my_ships.erase(move_ship->id);
        find_move = true;
    }
    else if ((destination != move_ship->position) && !my_ships.empty()) // moved
    {
        find_move = true;
        if ((this->ship_map[this->my_id][destination.y][destination.x] >= 0) &&
            (my_ships.find(this->ship_map[this->my_id][destination.y][destination.x]) != my_ships.end()))
        {
            // move ship gets set in ship_at_position
            move_ship = my_ships.find(this->ship_map[this->my_id][destination.y][destination.x])->second;
            ship_must_move = true;
            my_ships.erase(move_ship->id);
        }
        else // !ship_at_position(next_pos, my_ships, move_ship)
        {
            while (my_ships.find(this->ship_queue.top().second->id) == my_ships.end())
            {
                this->ship_queue.pop();
            }
            ship_must_move = false;
            move_ship = this->ship_queue.top().second;
            this->ship_queue.pop();
            my_ships.erase(move_ship->id);
        }
    }
    else // my_ships.empty()
    {
        ship_must_move = false;
        find_move = false;
    }
    return;
}


Direction BotInfo::_navigate(const Position& start, const Position& end, Halite& path_cost, int& path_dist, const EntityId& ship_id, const bool& safe_mode)
{
    priority_queue<direction_cost_T, vector<direction_cost_T>, greater<direction_cost_T>> neighbors;

    for (const Direction dir : ALL_CARDINALS)
    {
        Position neighbor = this->_normalize(start.directional_offset(dir));

        if (this->_euclidean_distance(neighbor, end) < this->_euclidean_distance(start, end))
        {
            if (this->ships_next_position[neighbor.y][neighbor.x] < 0)
            {
                bool enemy_ship = false;
                for (int i = 0; i < (int)this->ship_map.size(); i++)
                {
                    if ((i != this->my_id) && (this->ship_map[i][neighbor.y][neighbor.x]) >= 0)
                    {
                        enemy_ship = true;
                        break;
                    }

                    if (!enemy_ship && safe_mode) // check 2 positions out when in safe mode
                    {
                        for (const Direction dir2 : ALL_CARDINALS)
                        {
                            Position neighbor2 = this->_normalize(neighbor.directional_offset(dir2));

                            for (int j = 0; j < (int)this->ship_map.size(); j++)
                            {
                                if ((j != this->my_id) && (this->ship_map[j][neighbor2.y][neighbor2.x]) >= 0)
                                {
                                    enemy_ship = true;
                                    break;
                                }
                            }

                            if (enemy_ship)
                            {
                                break;
                            }
                        }
                    }
                }

                if (!enemy_ship || (this->_calculate_distance(neighbor, end) <= 5))
                {
                    neighbors.push(make_pair(this->halite_map[neighbor.y][neighbor.x], dir));
                }
            }
        }
    }

    if (!neighbors.empty())
    {
        Direction dir = neighbors.top().second;
        Position p = this->_normalize(start.directional_offset(dir));
        path_dist += 1;
        path_cost += round(this->halite_map[p.y][p.x] / (float)constants::MOVE_COST_RATIO);
        this->_retrace_path(p, end, path_cost, path_dist);
        return dir;
    }
    else
    {
        path_dist = 0;
        path_cost = 0;
        return Direction::STILL;
    }
}


Halite BotInfo::_get_stay_benefit(const Position& pos, const int& stay_time)
{
    Halite stay_benefit = 0;
    Halite recur_hal = this->halite_map[pos.y][pos.x];
    bool inspired = this->inspired_map[pos.y][pos.x];

    for (int i = 0; i < stay_time; i++)
    {
        stay_benefit += round(recur_hal / (float)constants::EXTRACT_RATIO * pow(3, inspired));
        recur_hal -= round(recur_hal / (float)constants::EXTRACT_RATIO);
    }
    return stay_benefit;
}


Position BotInfo::_find_target(const shared_ptr<Ship>& ship)
{
    priority_queue<desirability_T> desirability;
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            bool untargetable_found = false;
            for (int j = 0; j < this->num_players; j++)
            {
                if ((this->ship_map[j][y][x] >= 0) && (j != this->my_id))
                {
                    untargetable_found = true;
                    break;
                }
            }

            if (!untargetable_found)
            {
                for (int j = 0; j < this->num_players; j++)
                {
                    if (dropoff_dist[j][y][x].first <= 0)
                    {
                        untargetable_found = true;
                        break;
                    }
                }
            }

            if (!untargetable_found && (this->ships_next_position[y][x] < 0) &&
                (Position(x, y) != ship->position) &&
                (currently_targeted.find(Position(x, y)) == currently_targeted.end()))
            {
                Halite neighboring_halite = 0;
                for (Position p : Position(x, y).get_surrounding_cardinals())
                {
                    p = this->_normalize(p);
                    neighboring_halite += this->halite_map[p.y][p.x];
                }

                Halite hal = this->halite_map[y][x]*pow(3, (int)this->inspired_map[y][x]);
                int dist = _calculate_distance(ship->position, Position(x, y));

                //float pos_weight = 0.2*(this->current_halite / (float)this->starting_halite) + 0.5;
                //float position_score = hal - 40*dist - turn_percent*25*drop_dist - 20*populated;
                float position_score = (0.6*neighboring_halite + 0.4*hal) / dist;
                //float position_score = pow(0.6, dist) * (0.6*neighboring_halite + 0.4*hal);
                desirability.push(make_pair(position_score, Position(x, y)));
            }
        }
    }

    Position ret_pos;
    if (desirability.empty())
    {
        ret_pos = ship->position;
    }
    else
    {
        ret_pos = desirability.top().second;
    }
    return ret_pos;
}


float BotInfo::_euclidean_distance(const Position& source, const Position& target)
{
    const auto& normalized_source = _normalize(source);
    const auto& normalized_target = _normalize(target);

    const int dx = abs(normalized_source.x - normalized_target.x);
    const int dy = abs(normalized_source.y - normalized_target.y);

    const int toroidal_dx = min(dx, this->width - dx);
    const int toroidal_dy = min(dy, this->height - dy);

    return sqrt(pow(toroidal_dx, 2) + pow(toroidal_dy, 2));
}


int BotInfo::_calculate_distance(const Position& source, const Position& target)
{
    const auto& normalized_source = _normalize(source);
    const auto& normalized_target = _normalize(target);

    const int dx = abs(normalized_source.x - normalized_target.x);
    const int dy = abs(normalized_source.y - normalized_target.y);

    const int toroidal_dx = min(dx, this->width - dx);
    const int toroidal_dy = min(dy, this->height - dy);

    return toroidal_dx + toroidal_dy;
}


Position BotInfo::_normalize(const Position& position)
{
    const int x = (position.x + this->width) % this->width;
    const int y = (position.y + this->height) % this->height;
    return { x, y };
}

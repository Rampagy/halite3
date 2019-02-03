#include "MyBot.hpp"

int main(int argc, char* argv[]) {
    unsigned int rng_seed;
    if (argc > 1) {
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
    } else {
        rng_seed = static_cast<unsigned int>(time(nullptr));
    }
    mt19937 rng(rng_seed);

    Game game;
    BotInfo bot_info(game.me->id, game.players.size(), game.game_map->width, game.game_map->height);

    // As soon as you call "ready" function below, the 2 second per turn timer will start.
    game.ready("Rampagy");

    for (;;)
    {
        game.update_frame();
        shared_ptr<Player> me = game.me;
        unique_ptr<GameMap>& game_map = game.game_map;
        vector<Command> command_queue;

        bot_info.update_per_turn();

        for (const auto& playa : game.players)
        {
            // must be called in this order
            bot_info.update_dropoff_map(playa->id, playa->dropoffs, playa->shipyard);
            bot_info.update_ship_map(playa->id, playa->ships, playa->halite);
        }

        bot_info.build_inspired_map(game_map);
        bot_info.build_dropoff(command_queue, me->ships);
        bot_info.make_move(command_queue, me->ships);
        bot_info.build_new_ship(command_queue);

        if (!game.end_turn(command_queue))
        {
            break;
        }
    }

    return 0;
}

// Entry point: opens the SQLite file (default lms.db, or argv[1]), wires the application and runs the UI.
#include <iostream>

#include "common/Clock.h"
#include "security/PasswordHasher.h"
#include "strategy/LatePenalty.h"
#include "ui/ConsoleApp.h"

int main(int argc, char** argv) {
    const std::string dbPath = argc > 1 ? argv[1] : "lms.db";
    try {
        lms::Database db(dbPath);
        db.initialiseSchema();
        lms::SodiumPasswordHasher hasher;
        lms::SystemClock clock;
        lms::StandardLatePenalty penalty;
        lms::AppContext ctx(db, hasher, clock, penalty);
        lms::ConsoleApp app(ctx, std::cin, std::cout, clock);
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what()
                  << "\nThe program could not start. Check the database file and its folder permissions.\n";
        return 1;
    }
}

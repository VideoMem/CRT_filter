//
// Created by sebastian on 18/4/20.
//

#ifndef SDL_CRT_FILTER_FILESYSTEM_TESTS_H
#define SDL_CRT_FILTER_FILESYSTEM_TESTS_H

#include <dirent.h>

bool verify_path(const std::string name) {
    DIR *dir;
    struct dirent *ent;
    if ( (dir = opendir(".")) != nullptr ) {
        while ( (ent = readdir(dir)) != nullptr ) {
            //printf("%s\n", ent->d_name );
            if( ent->d_name == name ) {
                closedir( dir );
                return true;
            }
        }
        closedir( dir );
    } else {
        perror("");
    }
    return false;
}

TEST_CASE( "Filesystem Tests", "[App][Filesystem]") {

    SECTION("current path has resources access") {
        REQUIRE(verify_path("resources"));
    }

}


#endif //SDL_CRT_FILTER_FILESYSTEM_TESTS_H

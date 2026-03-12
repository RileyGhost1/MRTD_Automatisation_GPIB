cmake_minimum_required(VERSION 3.10)
project(MonProjetGTK)

# Trouver le paquet GTK4
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

# Créer l'exécutable
add_executable(mon_app main.c)

# Lier GTK4 à l'exécutable (gère les includes et les libs)
target_link_libraries(mon_app PkgConfig::GTK4)
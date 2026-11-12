#define NOB_IMPLEMENTATION
#include "nob.h"
#include <stdint.h>

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

static uint8_t debug = 0;
static uint8_t run_after_build = 0;

int main(int argc, char** argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
  if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, "gcc", "-Wall", "-Wextra");
  if (debug) {
    nob_cmd_append(&cmd, "-g", "-O0");
  } else {
    nob_cmd_append(&cmd, "-O2");
  }

  nob_cmd_append(&cmd, "-o", BUILD_FOLDER "main", SRC_FOLDER "main.c");

  if (!nob_cmd_run(&cmd)) return 1;

  if (run_after_build) {
    nob_cmd_append(&cmd, "./" BUILD_FOLDER "main");
    if (!nob_cmd_run(&cmd)) return 1;
  }

  return 0;
}


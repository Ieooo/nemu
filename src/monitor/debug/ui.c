#include <isa.h>
#include <memory/paddr.h>
#include "expr.h"
#include "watchpoint.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
int is_batch_mode();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_info(char *args) {
    if (args != NULL && strcmp(args, "register") != 0 && strcmp(args, "r") != 0) {
        printf("Undefined info command: \"%s\"\n", args);
        return 0;
    }
    isa_reg_display();
    return 0;
}

static int cmd_x(char *args) {
  paddr_t addr = atoi(args);
  if (addr < 0 || addr >= PMEM_SIZE) {
    printf("invalid memory addr [%s]\n", args);
    return 0;
  }
  word_t data = paddr_read(addr, 2);
  printf("%x\n", data);
  return 0;
}

static int cmd_si(char *args) {
  if (!args) {
    return 0;
  }

  int steps = atoi(args);
  if (steps <= 0 ) {
    printf("invalid args \"%s\"\n", args);
    return 0;
  }

  for(int i=0; i<steps; i++) {
    cpu_exec(-1);
  }
  return 0;
}

static int cmd_p(char *args) {
  if (!args) {
    return 0;
  }
  bool success = true;
  word_t res = expr(args, &success);
  if (success) {
    printf("%d\n", res);
  }
  return 0;
}

static int cmd_w(char *args) {
  return 0;
}

static int cmd_d(char *args) {
  return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "info", "Print register's infomation", cmd_info},
  {"x", "Print memory", cmd_x},
  {"si", "exec N step then pause", cmd_si},
  {"w", "set monitor point", cmd_w},
  {"d", "delete monitor point", cmd_d},
  {"p", "calculate expression value", cmd_p}

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop() {
  if (is_batch_mode()) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

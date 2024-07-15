#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>

struct device_change {
  char *device;
  char *action;
};

struct device_change get_state_difference(const char *state_file);

char *str_replace(const char *str, const char *old, const char *new) {
    char *result;
    int i, count = 0;
    int newlen = strlen(new);
    int oldlen = strlen(old);

    for (i = 0; str[i] != '\0'; i++) {
        if (strstr(&str[i], old) == &str[i]) {
            count++;
            i += oldlen - 1;
        }
    }

    result = (char *)malloc(i + 1 + count * (newlen - oldlen));

    i = 0;
    while (*str) {
        if (strstr(str, old) == str) {
            strcpy(&result[i], new);
            i += newlen;
            str += oldlen;
        } else
            result[i++] = *str++;
    }

    result[i] = '\0';
    return result;
}

char *get_home_dir() {
  char *home_dir = getenv("HOME");
  if (!home_dir) {
    struct passwd *pw = getpwuid(getuid());
    home_dir = pw->pw_dir;
  }
  return home_dir;
}

char *get_state_file(const char *state_file) {
  if (state_file) {
    return strdup(state_file);
  } else {
    char *home_dir = get_home_dir();
    char *path = malloc(strlen(home_dir) + strlen("/.config/usb.state") + 1);
    strcpy(path, home_dir);
    strcat(path, "/.config/usb.state");
    return path;
  }
}

void read_usb_devices() {
  system("lsusb > /tmp/usb-see-devices");
}

void freeze_usb_state(const char *state_file) {
  char *path = get_state_file(state_file);
  read_usb_devices();
  char command[512];
  snprintf(command, sizeof(command), "mv /tmp/usb-see-devices %s", path);
  system(command);
}

int scan_usb_state(const char *state_file) {
  struct device_change change = get_state_difference(state_file);
  if (!change.device || !change.action) return 0;
  printf("%s %s", change.action, change.device);

  if (strcmp(change.action, "added") == 0) {
    return 1;
  } else if (strcmp(change.action, "removed") == 0) {
    return 2;
  } else {
    return 0;
  }
}

struct device_change get_state_difference (const char *state_file) {
  char *path = get_state_file(state_file);
  read_usb_devices();

  FILE *frozen_file = fopen(path, "r");
  FILE *current_file = fopen("/tmp/usb-see-devices", "r");

  if (!frozen_file || !current_file) {
    perror("Error opening file");
    if (frozen_file) fclose(frozen_file);
    if (current_file) fclose(current_file);
    exit(-1);
  }

  int found;
  char frozen_line[1024];
  char current_line[1024];
  char *device_line = NULL;
  struct device_change change = {NULL, NULL};

  while (fgets(frozen_line, sizeof(frozen_line), frozen_file)) {
    found = 0;
    rewind(current_file);
    while (fgets(current_line, sizeof(current_line), current_file)) {
      if (strcmp(frozen_line, current_line) == 0) {
        found = 1;
        break;
      }
    }
    if (!found) {
      device_line = malloc(strlen(frozen_line) + 1);
      strcpy(device_line, frozen_line);
      change.device = device_line;
      change.action = "removed";
      break;
    }
  }

  if (!device_line) {
    rewind(frozen_file);
    rewind(current_file);
    while (fgets(current_line, sizeof(current_line), current_file)) {
      found = 0;
      rewind(frozen_file);
      while (fgets(frozen_line, sizeof(frozen_line), frozen_file)) {
        if (strcmp(frozen_line, current_line) == 0) {
          found = 1;
          break;
        }
      }
      if (!found) {
        device_line = malloc(strlen(current_line) + 1);
        strcpy(device_line, current_line);
        change.device = device_line;
        change.action = "added";
        break;
      }
    }
  }

  fclose(frozen_file);
  fclose(current_file);
  return change;
}

void watch_usb_state(const char *exec_command, const char *state_file) {
  freeze_usb_state(state_file);
  int initial_state = scan_usb_state(state_file);

  while (1) {
    sleep(1);
    int current_state = scan_usb_state(state_file);
    if (current_state != initial_state) {
      struct device_change change = get_state_difference(state_file);
      char command[1024];

      if (change.device) {
          char *replaced_usb = str_replace(exec_command, "_USB_", change.device);
          snprintf(command, sizeof(command), "%s", replaced_usb);
          free(replaced_usb);
      } else {
          snprintf(command, sizeof(command), "%s", exec_command); 
      }

      if (change.action) {
          char *replaced_action = str_replace(command, "_ACTION_", change.action);
          snprintf(command, sizeof(command), "%s", replaced_action);
          free(replaced_action);
      }

      system(command);
      freeze_usb_state(state_file);
      initial_state = scan_usb_state(state_file);
    }
  }
}

int main(int argc, char *argv[]) {
  const char *usage = "Usage: %s [options] <freeze|scan|watch>\n"
                      "\n"
                      "Commands:\n"
                      "  freeze  Freeze the current USB state\n"
                      "  scan    Scan the USB state for changes\n"
                      "  watch   Watch the USB state and execute a command when a change is detected\n"
                      "\n"
                      "Options:\n"
                      "  --file, -f <file>  Specify the state file to use\n"
                      "  --exec, -e <cmd>   Specify the command to execute\n"
                      "      Use _ACTION_ to get the action (added/removed)\n"
                      "      Use _USB_ to get the USB device\n"
                      "      Example: --exec 'zenity --info --text \"_USB_ was _ACTION_\"'\n";

  char *state_file = NULL;
  char *exec_command = NULL;
  int command_index = -1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--file") == 0 || strcmp(argv[i], "-f") == 0) {
      if (i + 1 < argc) {
        state_file = argv[++i];
      } else {
        fprintf(stderr, "Error: --file option requires an argument\n");
        return 255;
      }
    } else if (strcmp(argv[i], "--exec") == 0 || strcmp(argv[i], "-e") == 0) {
      if (i + 1 < argc) {
        exec_command = argv[++i];
      } else {
        fprintf(stderr, "Error: --exec option requires an argument\n");
        return 255;
      }
    } else {
      command_index = i;
      break;
    }
  }

  if (command_index == -1) {
    fprintf(stderr, usage, argv[0]);
    return 255;
  }

  if (command_index != -1) {
    if (strcmp(argv[command_index], "freeze") == 0) {
      freeze_usb_state(state_file);
    } else if (strcmp(argv[command_index], "scan") == 0) {
      return scan_usb_state(state_file);
    } else if (strcmp(argv[command_index], "watch") == 0) {
      if (exec_command == NULL) exec_command = "false";
      watch_usb_state(exec_command, state_file);
    } else {
      fprintf(stderr, usage, argv[0]);
      fprintf(stderr, "\nInvalid command: %s\n", argv[command_index]);
      return 255;
    }
  } else {
    fprintf(stderr, usage, argv[0]);
    return 255;
  }

  return 0;
}

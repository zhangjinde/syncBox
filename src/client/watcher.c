#include "dropboxClient.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE))

void *watcher_thread(void* ptr_path) {
  char* watch_path = malloc(strlen((char*) ptr_path));
  strcpy(watch_path, (char*) ptr_path);

  int fd, wd;
  int length, i = 0;
  char buffer[EVENT_BUF_LEN];

  fd = inotify_init();
  if(fd < 0) {
    perror("inotify_init");
  }

  wd = inotify_add_watch(fd, watch_path, IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM | IN_MOVED_TO);

  char path[MAXNAME];
  int thread_running = TRUE;
  while(thread_running) {
    length = read(fd, buffer, EVENT_BUF_LEN);

    if (length < 0) {
      thread_running = FALSE;
    } else {
      i = 0;
      while (i < length) {
        struct inotify_event* event = (struct inotify_event *) &buffer[i];

        if (event->len) {
          sprintf(path, "%s/%s", watch_path, event->name);

          if (event->mask & (IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO)) {
            if (fileExists(path) && (event->name[0] != '.')) {
              printf("Request upload: %s\n", path);
              send_file(path);
            }
          } else if (event->mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM)) {
            if (event->name[0] != '.') {
              printf("Request delete: %s\n", path);
              delete_file(path);
            }
          }
        }

        i += EVENT_SIZE + event->len;
      }
    }

    usleep(100);
  }

  inotify_rm_watch(fd, wd);
  close(fd);

  return 0;
}
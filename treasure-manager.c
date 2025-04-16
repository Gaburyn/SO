#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define MAX_PATH 256
#define MAX_CLUE_TEXT 512

typedef struct {
    int treasure_id;
    char username[64];
    double latitude;
    double longitude;
    char clue_text[MAX_CLUE_TEXT];
    int value;
} Treasure;

void add_treasure(const char *hunt_id);
void list_treasures(const char *hunt_id);
void view_treasure(const char *hunt_id, const char *treasure_id);
void remove_treasure(const char *hunt_id, const char *treasure_id);
void remove_hunt(const char *hunt_id);
void log_operation(const char *hunt_id, const char *operation);
void create_symbolic_link(const char *hunt_id);
int create_hunt_directory(const char *hunt_id);
int get_next_treasure_id(const char *hunt_id);
void display_treasure(Treasure *treasure);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [--add|--list|--view|--remove_treasure|--remove_hunt] [hunt_id] [treasure_id]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        if (argc < 3) {
            printf("Usage: %s --add [hunt_id]\n", argv[0]);
            return 1;
        }
        add_treasure(argv[2]);
    } 
    else if (strcmp(argv[1], "--list") == 0) {
        if (argc < 3) {
            printf("Usage: %s --list [hunt_id]\n", argv[0]);
            return 1;
        }
        list_treasures(argv[2]);
    } 
    else if (strcmp(argv[1], "--view") == 0) {
        if (argc < 4) {
            printf("Usage: %s --view [hunt_id] [treasure_id]\n", argv[0]);
            return 1;
        }
        view_treasure(argv[2], argv[3]);
    } 
    else if (strcmp(argv[1], "--remove_treasure") == 0) {
        if (argc < 4) {
            printf("Usage: %s --remove_treasure [hunt_id] [treasure_id]\n", argv[0]);
            return 1;
        }
        remove_treasure(argv[2], argv[3]);
    } 
    else if (strcmp(argv[1], "--remove_hunt") == 0) {
        if (argc < 3) {
            printf("Usage: %s --remove_hunt [hunt_id]\n", argv[0]);
            return 1;
        }
        remove_hunt(argv[2]);
    } 
    else {
        printf("Unknown command: %s\n", argv[1]);
        printf("Usage: %s [--add|--list|--view|--remove_treasure|--remove_hunt] [hunt_id] [treasure_id]\n", argv[0]);
        return 1;
    }

    return 0;
}

int create_hunt_directory(const char *hunt_id) {
    char directory_path[MAX_PATH];
    snprintf(directory_path, MAX_PATH, "./%s", hunt_id);

    struct stat st = {0};
    if (stat(directory_path, &st) == -1) {
        if (mkdir(directory_path, 0755) == -1) {
            perror("Could not create hunt directory");
            return 0;
        }
        printf("Created a new hunt directory: %s\n", hunt_id);
    }
    return 1;
}

void log_operation(const char *hunt_id, const char *operation) {
    char log_path[MAX_PATH];
    snprintf(log_path, MAX_PATH, "./%s/logged_hunt", hunt_id);
    
    int log_fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd == -1) {
        perror("Could not open log file");
        return;
    }
    
    time_t now = time(NULL);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    char log_entry[MAX_PATH + 128];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", timestamp, operation);
    
    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);
}

void create_symbolic_link(const char *hunt_id) {
    char log_path[MAX_PATH];
    char link_path[MAX_PATH];
    
    snprintf(log_path, MAX_PATH, "./%s/logged_hunt", hunt_id);
    snprintf(link_path, MAX_PATH, "./logged_hunt-%s", hunt_id);
    
    unlink(link_path);
    
    if (symlink(log_path, link_path) == -1) {
        perror("Could not create symbolic link");
    }
}

int get_next_treasure_id(const char *hunt_id) {
    char data_path[MAX_PATH];
    snprintf(data_path, MAX_PATH, "./%s/treasures.bin", hunt_id);
    
    int fd = open(data_path, O_RDONLY);
    if (fd == -1) {
        return 1;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Could not get file stats");
        close(fd);
        return 1;
    }
    
    int treasure_count = st.st_size / sizeof(Treasure);
    
    if (treasure_count == 0) {
        close(fd);
        return 1;
    }
    
    Treasure last_treasure;
    if (lseek(fd, (treasure_count - 1) * sizeof(Treasure), SEEK_SET) == -1) {
        perror("Could not seek to last treasure");
        close(fd);
        return 1;
    }
    
    if (read(fd, &last_treasure, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("Could not read last treasure");
        close(fd);
        return 1;
    }
    
    close(fd);
    return last_treasure.treasure_id + 1;
}

void add_treasure(const char *hunt_id) {
    if (!create_hunt_directory(hunt_id)) {
        return;
    }
    
    char data_path[MAX_PATH];
    snprintf(data_path, MAX_PATH, "./%s/treasures.bin", hunt_id);
    
    int fd = open(data_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("Could not open data file");
        return;
    }
    
    Treasure new_treasure;
    new_treasure.treasure_id = get_next_treasure_id(hunt_id);
    
    printf("Enter username: ");
    scanf("%63s", new_treasure.username);
    
    printf("Enter latitude: ");
    scanf("%lf", &new_treasure.latitude);
    
    printf("Enter longitude: ");
    scanf("%lf", &new_treasure.longitude);
    
    printf("Enter clue text: ");
    getchar();
    fgets(new_treasure.clue_text, MAX_CLUE_TEXT, stdin);
    size_t len = strlen(new_treasure.clue_text);
    if (len > 0 && new_treasure.clue_text[len-1] == '\n') {
        new_treasure.clue_text[len-1] = '\0';
    }
    
    printf("Enter value: ");
    scanf("%d", &new_treasure.value);
    
    if (write(fd, &new_treasure, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("Could not write treasure data");
        close(fd);
        return;
    }
    
    close(fd);
    
    char log_msg[MAX_PATH];
    snprintf(log_msg, MAX_PATH, "Added treasure with ID %d by user %s", 
             new_treasure.treasure_id, new_treasure.username);
    log_operation(hunt_id, log_msg);
    
    create_symbolic_link(hunt_id);
    
    printf("Treasure has been added with ID: %d\n", new_treasure.treasure_id);
}

void display_treasure(Treasure *treasure) {
    printf("Treasure ID: %d\n", treasure->treasure_id);
    printf("Username: %s\n", treasure->username);
    printf("Location: %.6f, %.6f\n", treasure->latitude, treasure->longitude);
    printf("Clue: %s\n", treasure->clue_text);
    printf("Value: %d\n", treasure->value);
}

void list_treasures(const char *hunt_id) {
    char data_path[MAX_PATH];
    snprintf(data_path, MAX_PATH, "./%s/treasures.bin", hunt_id);
    
    struct stat st;
    if (stat(data_path, &st) == -1) {
        if (errno == ENOENT) {
            printf("Hunt %s has no treasures or doesn't exist.\n", hunt_id);
        } else {
            perror("Error getting file stats");
        }
        return;
    }
    
    int fd = open(data_path, O_RDONLY);
    if (fd == -1) {
        perror("Could not open data file");
        return;
    }
    
    printf("Hunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", (long)st.st_size);
    
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
    printf("Last modified: %s\n", time_str);
    
    int treasure_count = st.st_size / sizeof(Treasure);
    printf("Number of treasures: %d\n\n", treasure_count);
    
    Treasure treasure;
    for (int i = 0; i < treasure_count; i++) {
        if (read(fd, &treasure, sizeof(Treasure)) != sizeof(Treasure)) {
            perror("Failed to read treasure data");
            close(fd);
            return;
        }
        
        printf("--- Treasure %d ---\n", i + 1);
        display_treasure(&treasure);
        printf("\n");
    }
    
    close(fd);
    
    log_operation(hunt_id, "Listed all treasures");
}

void view_treasure(const char *hunt_id, const char *treasure_id_str) {
    int treasure_id = atoi(treasure_id_str);
    
    char data_path[MAX_PATH];
    snprintf(data_path, MAX_PATH, "./%s/treasures.bin", hunt_id);
    
    int fd = open(data_path, O_RDONLY);
    if (fd == -1) {
        perror("Could not open data file");
        return;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file stats");
        close(fd);
        return;
    }
    
    int treasure_count = st.st_size / sizeof(Treasure);
    
    Treasure treasure;
    int found = 0;
    
    for (int i = 0; i < treasure_count; i++) {
        if (read(fd, &treasure, sizeof(Treasure)) != sizeof(Treasure)) {
            perror("Could not read treasure data");
            close(fd);
            return;
        }
        
        if (treasure.treasure_id == treasure_id) {
            found = 1;
            break;
        }
    }
    
    close(fd);
    
    if (found) {
        printf("--- Treasure Details ---\n");
        display_treasure(&treasure);
        
        char log_msg[MAX_PATH];
        snprintf(log_msg, MAX_PATH, "Viewed treasure with ID %d", treasure_id);
        log_operation(hunt_id, log_msg);
    } else {
        printf("Treasure with ID %d not found in hunt %s.\n", treasure_id, hunt_id);
    }
}

void remove_treasure(const char *hunt_id, const char *treasure_id_str) {
    int treasure_id = atoi(treasure_id_str);
    
    char data_path[MAX_PATH];
    char temp_path[MAX_PATH];
    snprintf(data_path, MAX_PATH, "./%s/treasures.bin", hunt_id);
    snprintf(temp_path, MAX_PATH, "./%s/treasures.tmp", hunt_id);
    
    int src_fd = open(data_path, O_RDONLY);
    if (src_fd == -1) {
        perror("Failed to open data file");
        return;
    }
    
    int dst_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        perror("Failed to create temp file");
        close(src_fd);
        return;
    }
    
    struct stat st;
    if (fstat(src_fd, &st) == -1) {
        perror("Error getting file stats");
        close(src_fd);
        close(dst_fd);
        return;
    }
    
    int treasure_count = st.st_size / sizeof(Treasure);
    int found = 0;
    
    Treasure treasure;
    for (int i = 0; i < treasure_count; i++) {
        if (read(src_fd, &treasure, sizeof(Treasure)) != sizeof(Treasure)) {
            perror("Failed to read treasure data");
            close(src_fd);
            close(dst_fd);
            unlink(temp_path);
            return;
        }
        
        if (treasure.treasure_id == treasure_id) {
            found = 1;
            continue;
        }
        
        if (write(dst_fd, &treasure, sizeof(Treasure)) != sizeof(Treasure)) {
            perror("Failed to write treasure data");
            close(src_fd);
            close(dst_fd);
            unlink(temp_path);
            return;
        }
    }
    
    close(src_fd);
    close(dst_fd);
    
    if (found) {
        if (rename(temp_path, data_path) == -1) {
            perror("Failed to update data file");
            unlink(temp_path);
            return;
        }
        
        printf("Treasure with ID %d removed from hunt %s.\n", treasure_id, hunt_id);
        
        char log_msg[MAX_PATH];
        snprintf(log_msg, MAX_PATH, "Removed treasure with ID %d", treasure_id);
        log_operation(hunt_id, log_msg);
        
        create_symbolic_link(hunt_id);
    } else {
        printf("Treasure with ID %d not found in hunt %s.\n", treasure_id, hunt_id);
        unlink(temp_path);
    }
}

void remove_hunt(const char *hunt_id) {
    char dir_path[MAX_PATH];
    char data_path[MAX_PATH];
    char log_path[MAX_PATH];
    char link_path[MAX_PATH];
    
    snprintf(dir_path, MAX_PATH, "./%s", hunt_id);
    snprintf(data_path, MAX_PATH, "./%s/treasures.bin", hunt_id);
    snprintf(log_path, MAX_PATH, "./%s/logged_hunt", hunt_id);
    snprintf(link_path, MAX_PATH, "./logged_hunt-%s", hunt_id);
    
    struct stat st;
    if (stat(dir_path, &st) == -1) {
        printf("Hunt %s doesn't exist.\n", hunt_id);
        return;
    }
    
    if (unlink(data_path) == -1 && errno != ENOENT) {
        perror("Could not delete data file");
        return;
    }
    
    if (unlink(log_path) == -1 && errno != ENOENT) {
        perror("Could not delete log file");
        return;
    }
    
    if (rmdir(dir_path) == -1) {
        perror("Could not delete hunt directory");
        return;
    }
    
    if (unlink(link_path) == -1 && errno != ENOENT) {
        perror("Could not delete symbolic link");
    }
    
    printf("Hunt %s has been removed.\n", hunt_id);
}

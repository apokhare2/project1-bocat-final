#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096  // 4KB buffer for I/O operations

/**
 * copy_file - Copies data from a given file descriptor to standard output.
 * @fd: The file descriptor to read from.
 * @name: The name of the file (or "stdin" if reading standard input), used in error messages.
 *
 * Uses read(2) to read data in fixed-size chunks into a buffer and write(2) to write data out.
 * Handles partial writes by looping until the entire chunk is written.
 *
 * Returns 0 on success, or -1 on error.
 */
static int copy_file(int fd, const char *name) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        ssize_t total_written = 0;
        while (total_written < bytes_read) {
            bytes_written = write(STDOUT_FILENO, buffer + total_written, bytes_read - total_written);
            if (bytes_written < 0) {
                // Report a write error for the given file (or "stdin").
                warn("%s", name);
                return -1;
            }
            total_written += bytes_written;
        }
    }
    if (bytes_read < 0) {
        // Report a read error for the given file.
        warn("%s", name);
        return -1;
    }
    return 0;
}

/**
 * main - Entry point for the bobcat program.
 * @argc: The number of command-line arguments.
 * @argv: The array of arguments.
 *
 * If no file operands are provided or an operand is "-", the program will read
 * from the standard input. Otherwise, it opens each file sequentially, copies its
 * contents to standard output, and reports any errors using warn(3). The program
 * exits with 0 on complete success, or a nonzero value if one or more errors occurred.
 */
int main(int argc, char *argv[]) {
    int exit_status = EXIT_SUCCESS;

    // If no arguments are provided, read from standard input.
    if (argc == 1) {
        if (copy_file(STDIN_FILENO, "stdin") < 0) {
            exit_status = EXIT_FAILURE;
        }
    } else {
        // Process each command-line operand in order.
        for (int i = 1; i < argc; i++) {
            // If the operand is "-" then treat it as standard input.
            if (strcmp(argv[i], "-") == 0) {
                if (copy_file(STDIN_FILENO, "stdin") < 0) {
                    exit_status = EXIT_FAILURE;
                }
            } else {
                // Open the file in read-only mode.
                int fd = open(argv[i], O_RDONLY);
                if (fd < 0) {
                    // Report error if the file cannot be opened; continue to next operand.
                    warn("%s", argv[i]);
                    exit_status = EXIT_FAILURE;
                    continue;
                }
                if (copy_file(fd, argv[i]) < 0) {
                    exit_status = EXIT_FAILURE;
                }
                // Close the file descriptor and report if an error occurs during closing.
                if (close(fd) < 0) {
                    warn("%s", argv[i]);
                    exit_status = EXIT_FAILURE;
                }
            }
        }
    }

    return exit_status;
}

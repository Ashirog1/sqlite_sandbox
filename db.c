#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

typedef struct {
  char* buffer;
  size_t buffer_length;
  __ssize_t input_length;
} InputBuffer;

typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMMAND
} MetaCommandResult;

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementType;

typedef enum {
  PREPARE_SUCCESS,
  PREPARE_UNRECOGNIZED_STATEMENT,
  PREPARE_SYNTAX_ERROR
} PrepareResult;

typedef struct {
  __uint32_t id;
  char username[COLUMN_USERNAME_SIZE];
  char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct {
  StatementType type;
  Row row_to_insert;
} Statement;

const __uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const __uint32_t ROW_SIZE;
const __uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const __uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
  __uint32_t num_rows;
  void* pages[TABLE_MAX_PAGES];
} Table;

const __uint32_t ID_SIZE = size_of_attribute(Row, id);
const __uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const __uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const __uint32_t ID_OFFSET = 0;
const __uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const __uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const __uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

InputBuffer* new_input_buffer() {
  InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;

  return input_buffer;
}

void serialize_row(Row* source, void* destination) {
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination) {
  memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

void* row_slot(Table* table, __uint32_t row_num) {
  __uint32_t page_num = row_num / ROWS_PER_PAGE;
  void* page = table->pages[page_num];

  if (page == NULL) {
    page = table->pages[page_num] = malloc(PAGE_SIZE);
  }
  __uint32_t row_offset = row_num % ROWS_PER_PAGE;
  __uint32_t byte_offset = row_offset * ROW_SIZE;
  return page + byte_offset;
}

void print_prompt() { printf("db > "); }

void read_input(InputBuffer* input_buffer) {
  __ssize_t bytes_read =
      getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
  if (bytes_read < 0) {
    printf("Error reading input \n");
    exit(EXIT_FAILURE);
  }

  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;
}

void close_input_buffer(InputBuffer* input_buffer) {
  free(input_buffer->buffer);
  free(input_buffer);
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMMAND;
  }
}

PrepareResult prepare_statement(InputBuffer* input_buffer,
                                Statement* statement) {
  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    statement->type = STATEMENT_INSERT;
    int args_assigned = sscanf(input_buffer->buffer, "insert %d %s %s",
                               &(statement->row_to_insert.id),
                               &(statement->row_to_insert.username),
                               &(statement->row_to_insert.email));
    if (args_assigned < 3) {
      return PREPARE_SYNTAX_ERROR;
    }
    return PREPARE_SUCCESS;
  }
  if (strcmp(input_buffer->buffer, "select") == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement* statement) {
  
}

int main(int argc, char* argv[]) {
  InputBuffer* input_buffer = new_input_buffer();
  while (true) {
    print_prompt();
    read_input(input_buffer);

    if (strcmp(input_buffer->buffer, ".exit") == 0) {
      if (input_buffer->buffer[0] == '.') {
        switch (do_meta_command(input_buffer)) {
          case META_COMMAND_SUCCESS:
            continue;
          case META_COMMAND_UNRECOGNIZED_COMMMAND:
            printf("Unrecognized command '%s'\n", input_buffer->buffer);
            continue;
        }
      }
    }

    Statement statement;
    switch (prepare_statement(input_buffer, &statement)) {
      case PREPARE_SUCCESS:
        break;
      case PREPARE_UNRECOGNIZED_STATEMENT:
        printf("Unrecognized statement '%s'\n", input_buffer->buffer);
        continue;
    }

    execute_statement(&statement);
    printf("Excuted.\n");
  }
}
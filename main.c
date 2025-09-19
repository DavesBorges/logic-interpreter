#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum TOKEN_KIND {
  TOKEN_TRUE = 'V',
  TOKEN_FALSE = 'F',
  TOKEN_VARIABLE = 'p',
  TOKEN_NEGATION = '~',
  TOKEN_IMPLIES = '>',
  TOKEN_AND = 'A',
  TOKEN_OR = 'v',
  TOKEN_LEFT = '(',
  TOKEN_RIGHT = ')',
  TOKEN_EOF = ';',
  TOKEN_LET = 's',
};

uint8_t conjunctionTTable[2][2] = {
    {0, 0},
    {0, 1},
};
uint8_t disjunctionTTable[2][2] = {
    {0, 1},
    {1, 1},
};
uint8_t impliesTTable[2][2] = {
    {1, 1},
    {0, 1},
};

struct Token {
  char kind;
  char rawValue;
  uint8_t boolValue;
};

struct TokenStream {
  int size;
  struct Token tokens[64];
  int readPosition;
  int writePosition;
};

struct Variable {
  uint8_t boolValue;
  char name;
  uint8_t valueAssigned;
};

struct Scope {
  struct Variable variables[8];
  int size;
};

struct Scope globalScope;

int assignVariable(struct Scope *scope, char name, uint8_t value) {
  if (scope->size == 8) {
    fprintf(stderr, "Cannot declare any more variables");
    return -1;
  }

  // Try to see if the variable is already declared
  for (int i = 0; i < scope->size; ++i) {
    if (scope->variables[i].name == name) {
      scope->variables[i].boolValue = value;
      return value;
    }
  }
  struct Variable variable;
  variable.name = name;
  variable.boolValue = value;
  scope->variables[scope->size++] = variable;
  return 0;
}

int getVariableValue(struct Scope *scope, char name) {

  // Try to see if the variable is already declared
  for (int i = 0; i < scope->size; ++i) {
    if (scope->variables[i].name == name) {
      return scope->variables[i].boolValue;
    }
  }

  return -1;
}

struct Token getNextToken(struct TokenStream *stream) {
  if (stream->readPosition == stream->writePosition) {
    struct Token eof;
    eof.kind = TOKEN_EOF;
    return eof;
  }

  return stream->tokens[stream->readPosition++];
}

int pushToken(struct TokenStream *stream, struct Token token) {
  if (stream->writePosition == stream->size) {
    return -1;
  }

  stream->tokens[stream->writePosition++] = token;
  return stream->writePosition;
}

int ungetLastToken(struct TokenStream *stream) {
  --stream->readPosition;
  return stream->readPosition;
}
int ungetToken(struct TokenStream *stream, struct Token token) {
  stream->tokens[--stream->readPosition] = token;
  return stream->readPosition;
}

uint8_t parseExpression(struct TokenStream *stream);

void tokenize(char *buffer, struct TokenStream *stream);

int performVariableAssignment(char *buffer, int n) {
  char localBuffer[24];
  char name;
  char value;

  int result = sscanf(buffer, "%c = %c", &name, &value);
  if (result <= 0) {
    return -1;
  }

  if (name < 'p' || name > 'z') {
    printf("Invalid name");
    return -1;
  }

  

  if (value != 'V' && value != 'F') {
    return -1;
  }

  if (value == 'V') {
    assignVariable(&globalScope, name, 1);
    return 1;
  }

  assignVariable(&globalScope, name, 0);
  return 0;
}

int main() {
  size_t limit = 1024;
  char *buffer = calloc(limit, sizeof(char));
  struct TokenStream stream;

  while (1) {
    stream.readPosition = 0;
    stream.writePosition = 0;
    stream.size = 64;

    printf("> ");
    size_t n = getline(&buffer, &limit, stdin);
    if (n == -1) {
      fprintf(stderr, "failed to read input");
      exit(-1);
    }

    // Check if there's an assignment operation
    if (strstr(buffer, "=") != NULL) {
      int value = performVariableAssignment(buffer, limit);
      if (value == -1) {
        fprintf(stderr, "Invalid variable assignemnt. Variables assignemnt "
                        "have this form: p = V \n");
      }

      continue;
    }

    tokenize(buffer, &stream);

    // Check if all variables are declared
    uint8_t continueMainLoop = 0;
    for (int i = stream.readPosition; i < stream.writePosition; ++i) {
      struct Token token = stream.tokens[i];
      if (token.kind == TOKEN_VARIABLE) {
        int value = getVariableValue(&globalScope, token.rawValue);
        if (value == -1) {
          fprintf(stderr, "Variable %c does not have a value assigned\n",
                  token.rawValue);
          stream.readPosition = 0;
          stream.writePosition = 0;
          continueMainLoop = 1;
          break;
        }
      }
    }
    if (continueMainLoop) {
      continue;
    }
    uint8_t boolValue = parseExpression(&stream);
    if (boolValue == 0) {
      printf("= F\n");
    } else {
      printf("= V\n");
    }
  }
}

uint8_t parsePrimary(struct TokenStream *stream) {
  struct Token token = getNextToken(stream);
  if (token.kind != TOKEN_VARIABLE && token.kind != TOKEN_TRUE &&
      token.kind != TOKEN_FALSE && token.kind != TOKEN_LEFT) {
    fprintf(stderr, "Expected primary but got %c\n", token.kind);
    exit(-1);
  }

  if (token.kind == TOKEN_LEFT) {
    uint8_t value = parseExpression(stream);
    token = getNextToken(stream);
    if (token.kind != TOKEN_RIGHT) {
      fprintf(stderr, "Missing closing parentesis");
      exit(-1);
    }

    return value;
  }

  if (token.kind == TOKEN_VARIABLE) {
    int variableValue = getVariableValue(&globalScope, token.rawValue);
    return variableValue;
  }
  return token.boolValue;
}
uint8_t parseNegation(struct TokenStream *stream) {
  struct Token token = getNextToken(stream);
  uint8_t value = 0;
  if (token.kind == TOKEN_NEGATION) {
    token = getNextToken(stream);
    if (token.kind == TOKEN_EOF) {
      fprintf(stderr, "missing negation operand");
      exit(1);
    }
    ungetToken(stream, token);
    value = parsePrimary(stream);
    return !value;
  }

  ungetToken(stream, token);
  value = parsePrimary(stream);

  return value;
}

uint8_t parseConjunction(struct TokenStream *stream) {
  uint8_t value = parseNegation(stream);
  struct Token token = getNextToken(stream);
  while (token.kind == TOKEN_AND) {

    token = getNextToken(stream);
    if (token.kind == TOKEN_EOF) {
      fprintf(stderr, "missing second operand");
      exit(1);
    }
    ungetToken(stream, token);
    uint8_t rightValue = parseConjunction(stream);
    value = conjunctionTTable[value][rightValue];
    token = getNextToken(stream);
  }
  if (token.kind != TOKEN_EOF) {
    ungetToken(stream, token);
  }
  return value;
}

uint8_t parseDisjunction(struct TokenStream *stream) {
  uint8_t value = parseConjunction(stream);
  struct Token token = getNextToken(stream);
  while (token.kind == TOKEN_OR) {
    token = getNextToken(stream);
    if (token.kind == TOKEN_EOF) {
      fprintf(stderr, "missing second operand");
      exit(1);
    }
    ungetToken(stream, token);
    uint8_t rightValue = parseDisjunction(stream);
    value = disjunctionTTable[value][rightValue];

    token = getNextToken(stream);
  }
  if (token.kind != TOKEN_EOF) {
    ungetToken(stream, token);
  }

  return value;
}

uint8_t parseExpression(struct TokenStream *stream) {
  uint8_t value = parseDisjunction(stream);
  struct Token token = getNextToken(stream);
  if (token.kind == TOKEN_EOF) {
    return value;
  }

  while (token.kind == TOKEN_IMPLIES) {
    token = getNextToken(stream);
    if (token.kind == TOKEN_EOF) {
      fprintf(stderr, "missing second operand");
      exit(1);
    }
    ungetToken(stream, token);
    uint8_t rightValue = parseExpression(stream);
    value = impliesTTable[value][rightValue];
    token = getNextToken(stream);
  }

  if (token.kind != TOKEN_EOF) {
    ungetToken(stream, token);
  }

  return value;
}
void tokenize(char *buffer, struct TokenStream *stream) {
  for (int i = 0; i < strlen(buffer); ++i) {
    char c = buffer[i];
    if (c == ' ' || c == '\n') {
      continue;
    }
    struct Token token;
    switch (c) {
    case TOKEN_TRUE:
      token.kind = c;
      token.boolValue = 1;
      pushToken(stream, token);
      break;
    case TOKEN_FALSE:
      token.kind = c;
      token.boolValue = 0;
      pushToken(stream, token);
      break;
    case TOKEN_OR:
    case TOKEN_AND:
    case TOKEN_LEFT:
    case TOKEN_RIGHT:
    case TOKEN_NEGATION:
      token.kind = c;
      token.boolValue = 0;
      pushToken(stream, token);
      break;
    case '-':
      i++;
      if (buffer[i] == '>') {

        token.kind = TOKEN_IMPLIES;
        pushToken(stream, token);
      }
      break;

    default:
      if (c >= 'p' && c < 'z') {
        token.kind = TOKEN_VARIABLE;
        token.rawValue = c;
        pushToken(stream, token);
      } else {
        printf("You typed a unrecognized token: %c\n", c);
      }
    }
  }
}

/*
 *

  fbf
  ( fbf)
  fbf AND fbf
  fbf OR fbf
  fbf -> fbf
*/

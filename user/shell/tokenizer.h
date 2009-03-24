/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <common.h>

#define ARG_BUFFER_SIZE 10
#define STR_BUFFER_SIZE 30

/**
 * All token-types
 */
typedef enum {
	TOK_ARGUMENT,
	TOK_RUN_IN_BG,
	TOK_END,
	TOK_PIPE
} eTokenType;

/**
 * Represents a token
 */
typedef struct {
	char *str;
	eTokenType type;
} sCmdToken;

/**
 * Prints the given tokens
 *
 * @param token the tokens
 * @param count the number of tokens
 */
void tok_printAll(sCmdToken *token,u32 count);

/**
 * Prints the given token
 *
 * @param token the token
 */
void tok_print(sCmdToken *token);

/**
 * Returns all tokens in the given string. That means the function walks through the string, separates it by ' ','\t',';','&','|' and '"'
 * and stores the found tokens. The number of found tokens will be written to tokenCount.
 * Note that you have to call freeTokens() if the return-value is NOT NULL after you're done!
 *
 * @param str the string
 * @param tokenCount a pointer on the number of found tokens (will be set by the function)
 * @return the found tokens or NULL if an error occurred
 */
sCmdToken *tok_get(char *str,u32 *tokenCount);

/**
 * Frees the tokens read from getTokens()
 *
 * @param argv the token-array
 * @param argc the number of tokens
 */
void tok_free(sCmdToken *argv,u32 argc);

#endif /* TOKENIZER_H_ */

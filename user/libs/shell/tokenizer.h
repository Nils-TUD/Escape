/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <esc/common.h>

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

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* TOKENIZER_H_ */

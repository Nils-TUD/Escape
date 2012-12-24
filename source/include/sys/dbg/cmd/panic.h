/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#ifndef DBG_PANIC_H_
#define DBG_PANIC_H_

#include <sys/common.h>

/**
 * Causes a panic
 *
 * @param argc the number of args
 * @param argv the arguments
 * @return 0 on success
 */
int cons_cmd_panic(size_t argc,char **argv);

#endif /* DBG_PANIC_H_ */

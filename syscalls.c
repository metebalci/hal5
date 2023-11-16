/*
 * SPDX-FileCopyrightText: 2023 Mete Balci
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Mete Balci
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/stat.h>

#include "hal5.h"

int _close(int fd) 
{
  assert (fd == 1);

  return -1;
}

int _lseek(int fd, char* ptr, int dir)
{
  assert (fd == 1);

  return -1;
}

int _read(int fd, char* ptr, int len)
{
  assert (fd == 1);

  return 0;
}

int _write(int fd, char* ptr, int len)
{
  assert (fd == 1);

  int cnt = len;

  while (cnt > 0) {
    hal5_console_write(*ptr++);
    cnt--;
  }

  return len;
}

__USED int _isatty(int fd)
{
  assert (fd == 1);

  return 1;
}

__USED int _fstat(int fd, struct stat *buf)
{
  assert (fd == 1);

  buf->st_mode = S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO;

  return 0;
}

int _getpid(void) 
{
  return -1;
}

void _kill()
{
  return;
}

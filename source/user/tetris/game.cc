/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <sys/keycodes.h>
#include <sys/esccodes.h>
#include <sys/time.h>
#include <esc/proto/speaker.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"

int Game::_level = 0;
int Game::_score = 0;
int Game::_lines = 0;
long Game::_ticks = 0;
Game::State Game::_state = Game::RUNNING;
int Game::_removed = 0;
UI *Game::_ui;
Grid *Game::_grid;
ipc::Speaker *Game::_spk;
Stone *Game::_cur = NULL;
Stone *Game::_next = NULL;
std::mutex Game::_mutex;

void Game::start(int cols,int rows,int size,bool sound) {
	_ui = new UI(cols,rows,size);
	_grid = new Grid(_ui->gridCols(),_ui->gridRows());
	struct stat info;
	if(sound && stat("/dev/speaker",&info) == 0) {
		try {
			_spk = new ipc::Speaker("/dev/speaker");
		}
		catch(const std::exception &e) {
			printe("%s",e.what());
		}
	}
	srand(time(NULL) * rdtsc());
	reset();
	_ui->start();
}

void Game::stop() {
	delete _cur;
	delete _next;
	delete _ui;
	delete _grid;
	delete _spk;
}

void Game::reset() {
	delete _cur;
	delete _next;
	_grid->reset();
	_cur = createStone();
	_next = createStone();
	_cur->place();
	_level = 1;
	_ticks = 0;
	_score = 0;
	_lines = 0;
	_state = RUNNING;
	_removed = -1;
	_ui->paintBorder();
	_ui->paintInfo(_next,_level,_score,_lines);
}

Stone *Game::createStone() {
	switch(rand() % 7) {
		case 0:
			return new IStone(*_grid);
		case 1:
			return new OStone(*_grid);
		case 2:
			return new JStone(*_grid);
		case 3:
			return new LStone(*_grid);
		case 4:
			return new SStone(*_grid);
		case 5:
			return new TStone(*_grid);
		default:
			return new ZStone(*_grid);
	}
}

void Game::handleKey(const ipc::UIEvents::Event &ev) {
	// automatically pause the game if our UI gets inactive
	if(ev.type == ipc::UIEvents::Event::TYPE_UI_INACTIVE && _state != GAMEOVER) {
		_state = PAUSED;
		update(true);
		return;
	}

	if(ev.type != ipc::UIEvents::Event::TYPE_KEYBOARD || (ev.d.keyb.modifier & STATE_BREAK))
		return;

	std::lock_guard<std::mutex> guard(_mutex);
	bool changed = false;
	bool all = false;
	if(_state == RUNNING) {
		switch(ev.d.keyb.keycode) {
			case VK_LEFT:
				changed |= _cur->left();
				break;
			case VK_RIGHT:
				changed |= _cur->right();
				break;
			case VK_UP:
				changed |= _cur->rotate();
				break;
			case VK_DOWN:
				if(!_cur->down()) {
					newStone();
					all = true;
				}
				changed = true;
				break;
			case VK_SPACE:
				changed |= _cur->drop();
				if(changed) {
					newStone();
					all = true;
				}
				break;
		}
	}

	switch(ev.d.keyb.keycode) {
		case VK_R:
			reset();
			changed = true;
			all = true;
			break;
		case VK_P:
			if(_state != GAMEOVER) {
				_state = _state == PAUSED ? RUNNING : PAUSED;
				changed = true;
				all = true;
			}
			break;
		case VK_Q:
			_ui->stop();
			break;
	}

	if(changed)
		update(all);
}

void Game::newStone() {
	// we remove the lines one by one during tick()
	_state = ANIMATION;
	_removed = 0;
}

void Game::update(bool all) {
	if(all) {
		_ui->paintBorder();
		_ui->paintInfo(_next,_level,_score,_lines);
	}
	_ui->paintGrid(*_grid);
	if(_state == GAMEOVER) {
		const char *lines[] = {
			"Game Over!",
			"",
			"Press R to restart"
		};
		_ui->paintMsgBox(lines,ARRAY_SIZE(lines));
	}
	else if(_state == PAUSED) {
		const char *lines[] = {
			"Paused"
		};
		_ui->paintMsgBox(lines,ARRAY_SIZE(lines));
	}
	_ui->update();
}

void Game::tick() {
	if(_state == GAMEOVER || _state == PAUSED)
		return;

	std::lock_guard<std::mutex> guard(_mutex);
	if(_state == ANIMATION) {
		if(_grid->removeFull()) {
			if(_spk) {
				_spk->beep(600,60);
				_spk->beep(200,60);
			}
			_removed++;
		}
		else {
			// no more lines to remove, so finish it
			switch(_removed) {
				case 1:
					_score += 1 * _level;
					break;
				case 2:
					_score += 3 * _level;
					break;
				case 3:
					_score += 6 * _level;
					break;
				case 4:
					_score += 10 * _level;
					break;
			}
			_lines += _removed;
			_state = RUNNING;

			// new stone
			delete _cur;
			_cur = _next;
			_next = createStone();
			if(!_cur->place()) {
				_state = GAMEOVER;
				if(_spk) {
					_spk->beep(1000,300);
					_spk->beep(600,300);
					_spk->beep(200,600);
				}
			}
		}
		update(true);
		return;
	}

	// tick and move line down, if necessary
	int speed = (LEVELS + 2) - _level;
	if((_ticks++ % speed) == 0) {
		if(!_cur->down())
			newStone();
		update(true);
	}

	// to next level?
	if((_ticks % 300) == 0 && _level < LEVELS)
		_level++;
}

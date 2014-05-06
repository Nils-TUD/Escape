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

#pragma once

#include <esc/common.h>
#include <mutex>
#include <vector>

/**
 * Manages the jobs, i.e. UI clients we forked.
 */
class JobMng {
	JobMng() = delete;

	struct Job {
		explicit Job() : id(), loginPid(), termPid() {
		}
		explicit Job(int _id,int _loginPid,int _termPid)
			: id(_id), loginPid(_loginPid), termPid(_termPid) {
		}

		int id;
		int loginPid;
		int termPid;
	};

public:
	/**
	 * Waits for ever for terminated jobs.
	 */
	static void wait();

	/**
	 * Requests a new id for a job.
	 *
	 * @return the id or -1 if there is no free id anymore
	 */
	static int getId();

	/**
	 * @return true if the given job exists
	 */
	static bool exists(int id) {
		std::lock_guard<std::mutex> guard(_mutex);
		return get(id) != NULL;
	}

	/**
	 * Adds the given job to the list
	 *
	 * @param id the job-id
	 * @param termPid the pid for the terminal
	 */
	static void add(int id,int termPid) {
		std::lock_guard<std::mutex> guard(_mutex);
		_jobs.push_back(Job(id,-1,termPid));
	}

	/**
	 * Sets the login pid for the given job
	 *
	 * @param id the job-id
	 * @param pid the login pid
	 */
	static void setLoginPid(int id,int pid) {
		std::lock_guard<std::mutex> guard(_mutex);
		Job *job = get(id);
		if(job)
			job->loginPid = pid;
	}

private:
	static Job *get(int id);
	static void terminate(int pid);

	static std::mutex _mutex;
	static std::vector<Job> _jobs;
};

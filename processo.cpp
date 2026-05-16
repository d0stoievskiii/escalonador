#include "processo.hpp"

std::string StateFlagsToString(uint32_t flags)
{
	std::string out;
	uint32_t recognized = 0;

	for (const auto& e : kProcessStateTable)
	{
		if ((flags & e.value) != 0)
		{
			if (!out.empty())
				out += "-";
			out += e.name;
			recognized |= e.value;
		}
	}

	uint32_t unknown = flags & ~recognized;
	if (unknown != 0)
	{
		if (!out.empty())
			out += ", ";
		out += "UnknownBits(0x" + std::to_string(unknown) + ")";
	}

	if (out.empty())
		out = "None";

	return out;
}

Prioridade Processo::get_prioridade() {
	return prioridade;
}

ProcessState Processo::get_state() {
	return estado;
}

ProcessState Processo::set_state(ProcessState s) {
	estado = s;
	return estado;
}

uint32_t Processo::time_left() {
	if (exec_phase == 1) {
		return cpu_time_1;
	}
	return cpu_time_2;
}

void Processo::_block_or_end() {
	if (exec_phase == 1) {
		if (cpu_time_2 > 0) {
			exec_phase = 2;
			estado = BLOQUEADO;
			/*
			codigo pra criar evento de E/S
			*/
			return;
		}
	}
	estado = FINALIZADO;
}

bool Processo::exec() {
	if (exec_phase == 1) {
		cpu_time_1--;
	} else {
		cpu_time_2--;
	}

	if (time_left() == 0) {
		_block_or_end();
		return false;
	}

	return true;
}

Metrics Processo::generate_metrics() {
	Metrics ret;
	ret.pid = pid;
	ret.arrival = arrival_time;
	ret.end = finish_time;
	ret.service_time = service_time;

	ret.turnaround = ret.end - ret.arrival;
	ret.normalized_turnaround = (float)ret.turnaround/ret.service_time;

	return ret;
}

uint32_t Processo::get_pid() {
	return pid;
}

size_t Processo::get_size() {
	return mbs_ram;
}


uint32_t Processo::set_arrival_time(uint32_t time) {
	arrival_time = time;
	return arrival_time;
}

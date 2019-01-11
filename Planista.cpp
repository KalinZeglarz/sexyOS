#include "Planista.h"
#include <list>
#include <iterator>
#include <cmath>
#include <sstream>

extern class PCB;
bool WywlaszczeniePCB = 0;

class Planista {
private:
	std::list<PCB> WaitingPCB;
	std::list<PCB>::iterator Rpcb, Wpcb;
	unsigned char trial = 0;
	int CounterMax=1;
public:
	std::list<PCB> ReadyPCB;
	Planista() {}
	~Planista() {}

	void Check() {
		if (trial == ReadyPCB.size()/2) {
			trial = 0;
		}
		trial++;
		for (Rpcb = ReadyPCB.begin(); Rpcb != ReadyPCB.end(); Rpcb++) {
			if (Rpcb->state == TERMINATED) {
				Rpcb = ReadyPCB.erase(Rpcb);
			}
			if (Rpcb->state != READY) {
				WaitingPCB.push_back(*Rpcb);
				Rpcb = WaitingPCB.erase(Rpcb);
			}
			else {
				SetPriority(*Rpcb);
			}
		}
		for (Wpcb = WaitingPCB.begin(); Wpcb != WaitingPCB.end(); Wpcb++) {
			if (Wpcb->state == READY) {
				AddProces(*Wpcb);
				Wpcb = WaitingPCB.erase(Wpcb);

			}
		}
		SortReadyPCB();
	}
	void AddProces(PCB Proces) {
		bool x = 0;
		if (Proces.state == READY) {
			if (ReadyPCB.size() == 0) {
				ReadyPCB.push_back(Proces);
			}
			else {
				for (Rpcb = ReadyPCB.begin(); Rpcb != ReadyPCB.end(); Rpcb++) {
					if (Proces.priority > Rpcb->priority) {
						ReadyPCB.insert(Rpcb, Proces);
						if (Rpcb == ReadyPCB.begin()) {				// jesli proces będzie na 1 miejscu
							WywlaszczeniePCB = 1;					// flaga i przeładowanie kontekstu
						}
						x = 1;
						break;
					}
				}
				if (x == 0) {
					ReadyPCB.push_back(Proces);
				}
			}
		}
		else {
			WaitingPCB.push_back(Proces);
		}
	}
	void RemoveProces(PCB &Proces) {
		for (Rpcb = ReadyPCB.begin(); Rpcb != ReadyPCB.end(); Rpcb++) {
			if (Rpcb->PID == Proces.PID) {
				Rpcb = ReadyPCB.erase(Rpcb);
			}
		}
		for (Wpcb = WaitingPCB.begin(); Wpcb != WaitingPCB.end(); Wpcb++) {
			if (Wpcb->PID == Proces.PID) {
				Wpcb = WaitingPCB.erase(Wpcb);
			}
		}
	}
	void SortReadyPCB() {
		bool x;
		for (int i = 0; i < ReadyPCB.size(); i++) {
			x = 0;
			Wpcb = ReadyPCB.begin();
			Wpcb++;
			for (Rpcb = ReadyPCB.begin(); Wpcb != ReadyPCB.end(); Rpcb++, Wpcb++) {
				if (Rpcb->priority > Wpcb->priority) {
					std::swap(*Rpcb, *Wpcb);
					x = 1;
				}
			}
			if (x == 0) {
				break;
			}
		}
	}

	void SetPriority(PCB &Proces) {
		float x = 0;
		Proces.last_counter = Proces.comand_counter - Proces.last_counter;
//			USTALENIE MNOZNIKA od najwiekszego skoku
		if (Proces.last_counter > CounterMax) {
			CounterMax = Proces.last_counter;
		}
		if (Proces.last_counter > 0) {
			x = 2 + (Proces.priority + Proces.last_counter * 10 / CounterMax)/2;
			Proces.priority = (int)x;
		}
		if (Proces.priority >= 10) {
			Proces.priority = 9;
		}
//			POSTARZANIE, jesli proces nie otrzymał przydzialu do procesora w kilku ostatnich sesjach
		if (Proces.priority > 1 && trial==ReadyPCB.size()/2 && Proces.last_counter==0){
			Proces.priority--;
		}
		Proces.last_counter = Proces.comand_counter;
	}
};
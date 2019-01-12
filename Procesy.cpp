#include "Procesy.h"
#include "MemoryManager.h"

unsigned int free_PID = 2;

void PCB::change_state(Process_state x)
{
	this->state = x;
}

void PCB::add_file_to_proc(const std::string& open_file)
{
	this->open_files.push_back(open_file);
}

void PCB::kill_all_childrens(MemoryManager & mm)
{

	for (auto& i : this->child_vector)
	{
		mm.kill(i.PID);
	}
	this->child_vector.clear(); //Tu nast�puje usuni�cie wszystkich dzieci

}



void PCB::Set_PID(int i)
{
	this->PID = i;
}

PCB* PCB::GET_kid(unsigned int PID)
{
	for (PCB& kid : this->child_vector) {
		if (kid.PID == PID) { return &kid; }
		for (PCB& grandkid : kid.child_vector) {
			if (grandkid.PID == PID) { return &grandkid; }
			for (PCB& ggrandkid : grandkid.child_vector) {
				if (ggrandkid.PID == PID) { return &ggrandkid; }


			}

		}
	}


	for (PCB& kid : this->child_vector) {
		if (kid.PID == PID) { return &kid; }
		else if (!kid.child_vector.empty()) {
			return kid.GET_kid(PID);
		}
	}
	return nullptr;

}
PCB* PCB::GET_kid(const std::string& nazwa)
{
	for (PCB& kid : this->child_vector) {
		if (kid.process_name == nazwa) { return &kid; }
		for (PCB& grandkid : kid.child_vector) {
			if (grandkid.process_name == nazwa) { return &grandkid; }
			for (PCB& ggrandkid : grandkid.child_vector) {
				if (ggrandkid.process_name == nazwa) { return &ggrandkid; }


			}

		}
	}


	for (PCB& kid : this->child_vector) {
		if (kid.PID == PID) { return &kid; }
		else if (!kid.child_vector.empty()) {
			return kid.GET_kid(PID);
		}
	}
	return nullptr;

}

bool PCB::find_kid(unsigned int PID)
{
	for (PCB kid : this->child_vector) {
		if (kid.PID == PID) { return true; }
		else if (!kid.child_vector.empty()) {
			if (kid.find_kid(PID))
				return kid.find_kid(PID);
		}
	}
	return false;
}
void PCB::display_allkids()
{
	int i = 0;
	for (PCB kid : this->child_vector) {
		if (kid.parent_proc->PID == 1)std::cout << "\t nazwa procesu " << kid.process_name << " PID procesu " << kid.PID << std::endl;
		else
			std::cout << "\t\t nazwa procesu " << kid.process_name << " PID procesu " << kid.PID << std::endl;

		if (!kid.child_vector.empty()) {
			std::cout << "\t"; kid.display_allkids(i);

		}



	}

	std::cout << std::endl;
}
void PCB::display_allkids(int a)
{
	a++;

	for (PCB kid : this->child_vector) {
		for (int i = 1; i < a; i++) {
			std::cout << "\t";
		}
		std::cout << "\t\t nazwa procesu " << kid.process_name << " PID procesu " << kid.PID << std::endl;

		if (!kid.child_vector.empty()) {

			std::cout << "\t"; kid.display_allkids(a);

		}



	}

	std::cout << std::endl;
}
void proc_tree::fork(PCB proc, const std::string& name, int rozmiar) {
	if (proc.PID == this->proc.PID) {//sprawdza czy id ojca si� zgadza i jestli tak przypisuje go do niego.

		proc.PID = free_PID;
		free_PID++;
		proc.parent_proc = &this->proc;
		const auto pages = static_cast<unsigned int>(ceil(rozmiar / 16.0));
		proc.pageList = mm->createPageList(rozmiar, proc.PID);
		proc.change_state(READY);
		proc.proces_size = pages * 16;


		this->proc.child_vector.push_back(proc);


	}
	else {
		if (this->proc.GET_kid(proc.PID)->PID == proc.PID) {
			int temp = proc.PID;

			proc.parent_proc = this->proc.GET_kid(temp);
			proc.PID = free_PID;
			this->proc.GET_kid(temp)->child_vector.push_back(proc);
			std::cout << " znaleziono ojca" << std::endl;
			free_PID++;
			const auto pages = static_cast<unsigned int>(ceil(rozmiar / 16.0));
			proc.pageList = mm->createPageList(rozmiar, proc.PID);
			proc.change_state(READY);
			proc.proces_size = pages * 16;


		}
		else {
			std::cout << "nie znaleziono ojca" << std::endl;
		}

	}

}
void proc_tree::fork(PCB proc, const std::string& name, const std::string& file_name, int rozmiar) {
	if (proc.PID == this->proc.PID) {//sprawdza czy id ojca si� zgadza i jestli tak przypisuje go do niego.
		proc.PID = free_PID;
		free_PID++;
		proc.parent_proc = &this->proc;

		const auto pages = static_cast<unsigned int>(ceil(rozmiar / 16.0));
		if (mm->loadProgram(file_name, rozmiar, proc.PID) == -1) {
			//exit()
			throw 1;// rzucam cos rzeby funckeje przerwac

		}
		proc.pageList = mm->createPageList(rozmiar, proc.PID);
		proc.change_state(READY);
		proc.proces_size = pages * 16;
		this->proc.child_vector.push_back(proc);
	}
	else {
		if (this->proc.GET_kid(proc.PID)->PID == proc.PID) {
			int temp = proc.PID;


			proc.parent_proc = this->proc.GET_kid(temp);
			proc.PID = free_PID;
			this->proc.GET_kid(temp)->child_vector.push_back(proc);
			std::cout << " znaleziono ojca" << std::endl;
			free_PID++;
			const auto pages = static_cast<unsigned int>(ceil(rozmiar / 16.0));
			if (mm->loadProgram(file_name, rozmiar, proc.PID) != 1) {
				//exit()
				throw 1;// rzucam cos rzeby funckeje przerwac

			}
			proc.pageList = mm->createPageList(rozmiar, proc.PID);
			proc.change_state(READY);
			proc.proces_size = pages * 16;


		}
		else {
			std::cout << "nie znaleziono ojca" << std::endl;
		}

	}

}

void proc_tree::fork(PCB proc, const std::string& name)
{
	if (proc.PID == this->proc.PID) {//sprawdza czy id ojca si� zgadza i jestli tak przypisuje go do niego.
		proc.PID = free_PID;
		free_PID++;
		proc.parent_proc = &this->proc;
		this->proc.child_vector.push_back(proc);
	}
	else {
		if (this->proc.GET_kid(proc.PID)->PID == proc.PID) {
			int temp = proc.PID;


			proc.parent_proc = this->proc.GET_kid(temp);
			proc.PID = free_PID;
			this->proc.GET_kid(temp)->child_vector.push_back(proc);
			std::cout << " znaleziono ojca" << std::endl;
			free_PID++;


		}
		else {
			std::cout << "nie znaleziono ojca" << std::endl;
		}
	}

}
/*void proc_tree::fork(PCB * proc, const std::string name, MemoryManager mm, int rozmiar, std::string open_file)
{
	if (proc.PID == this->proc.PID) {//sprawdza czy id ojca si� zgadza i jestli tak przypisuje go do niego.

		proc.PID = free_PID;
		free_PID++;
		proc.parent_proc = &this->proc;
		proc.add_file_to_proc(open_file);
		const auto pages = static_cast<unsigned int>(ceil(rozmiar / 16.0));
		proc.pageList = mm.createPageList(rozmiar, proc.PID);
		proc.change_state(READY);
		proc.proces_size = pages * 16;
		this->proc.child_vector.push_back(proc);


	}
	else {
		if (this->proc.GET_kid(proc.PID)->PID == proc.PID) {
			int temp = proc.PID;


			proc.parent_proc = this->proc.GET_kid(temp);
			proc.PID = free_PID;
			this->proc.GET_kid(temp)->child_vector.push_back(proc);
			std::cout << " znaleziono ojca" << std::endl;
			free_PID++;
			const auto pages = static_cast<unsigned int>(ceil(rozmiar / 16.0));
			proc.pageList = mm.createPageList(rozmiar, proc.PID);
			proc.change_state(READY);
			proc.proces_size = pages * 16;
			proc.add_file_to_proc(open_file);

		}
		else {
			std::cout << "nie znaleziono ojca" << std::endl;
		}

	}
}*/
/*void proc_tree::fork(PCB *proc, const std::string name, std::string file_name, MemoryManager mm, int rozmiar) {
	if (proc.PID == this->proc.PID) {//sprawdza czy id ojca si� zgadza i jestli tak przypisuje go do niego.

		proc.PID = free_PID;
		free_PID++;
		proc.parent_proc = &this->proc;

		const auto pages = static_cast<unsigned int>(ceil(rozmiar / 16.0));// tu zaczyna sie ustawianie pami�ci
		proc.pageList = mm.createPageList(rozmiar, proc.PID);
		proc.change_state(READY);
		proc.proces_size = pages * 16;
		this->proc.child_vector.push_back(proc);


	}
	else {
		if (this->proc.GET_kid(proc.PID)->PID == proc.PID) {
			int temp = proc.PID;


			proc.parent_proc = this->proc.GET_kid(temp);
			proc.PID = free_PID;
			this->proc.GET_kid(temp)->child_vector.push_back(proc);
			std::cout << " znaleziono ojca" << std::endl;
			free_PID++;
			const auto pages = static_cast<unsigned int>(ceil(rozmiar / 16.0));
			proc.pageList = mm.createPageList(rozmiar, proc.PID);
			proc.change_state(READY);
			proc.proces_size = pages * 16;


		}
		else {
			std::cout << "nie znaleziono ojca" << std::endl;
		}
	}

}

/*void proc_tree::fork(PCB * proc, const std::string name, std::string file_name) tu jest bez mm
{
	{
		if (proc.PID == this->proc.PID) {//sprawdza czy id ojca si� zgadza i jestli tak przypisuje go do niego.
			proc.open_files.push_back(file_name);
			proc.PID = free_PID;
			free_PID++;
			proc.parent_proc = &this->proc;
			this->proc.child_vector.push_back(proc);

		}
		else {
			if (this->proc.GET_kid(proc.PID)->PID == proc.PID) {
				int temp = proc.PID;
				proc.open_files.push_back(file_name);
				proc.parent_proc = this->proc.GET_kid(temp);
				proc.PID = free_PID;
				this->proc.GET_kid(temp)->child_vector.push_back(proc);
				std::cout << " znaleziono ojca" << std::endl;
				free_PID++;

				;
			}
			else {
				std::cout << "nie znaleziono ojca" << std::endl;
			}
		}
	}
}*/
void proc_tree::fork(PCB proc, const std::string& name, std::vector<std::string> file_names)
{
	{
		if (proc.PID == this->proc.PID) {//sprawdza czy id ojca si� zgadza i jestli tak przypisuje go do niego.
			for (const std::string& file_name : file_names) {
				proc.open_files.push_back(file_name);
			}
			proc.PID = free_PID;
			free_PID++;
			proc.parent_proc = &this->proc;
			this->proc.child_vector.push_back(proc);

		}
		else {
			if (this->proc.GET_kid(proc.PID)->PID == proc.PID) {
				int temp = proc.PID;
				for (const std::string& file_name : file_names) {
					proc.open_files.push_back(file_name);
				}
				proc.parent_proc = this->proc.GET_kid(temp);
				proc.PID = free_PID;
				this->proc.GET_kid(temp)->child_vector.push_back(proc);
				std::cout << " znaleziono ojca" << std::endl;
				free_PID++;


			}
			else {
				std::cout << "nie znaleziono ojca" << std::endl;
			}
		}
	}
}

void proc_tree::exit(int pid)
{
	if (pid == this->proc.PID) {// kiedy damy id=1 
		std::cout << "nie mo�na usun�� inita/systemd" << std::endl;
	}
	else {
		if (this->proc.GET_kid(pid) == nullptr) {//jak nie znajdzie dziecka
			std::cout << "nie ma takiego procesu" << std::endl;
		}
		else {
			PCB* temp = this->proc.GET_kid(pid);
			if (temp->child_vector.empty()) {//proces nie ma dzieci
				for (size_t i = 0; i < temp->parent_proc->child_vector.size(); i++) {
					if (temp->parent_proc->child_vector.at(i).PID == pid) {
						mm->kill(pid);
						PCB* parent = temp->parent_proc;
						parent->child_vector.erase(parent->child_vector.begin() + i);
						break;
					}
				}
			}
			else { //kiedy ma dzieci
				//kiedy dziecko ma dzieci
				for (auto& i : temp->child_vector) {
					if (!i.child_vector.empty())//kiedy jest patologia
					{
						for (auto& j : i.child_vector)
							j.kill_all_childrens(*mm);
					}
					i.kill_all_childrens(*mm);
				}
			}
		}
	}
}





void proc_tree::display_tree()
{
	std::cout << "nazwa procesu " << proc.process_name << " PID procesu " << proc.PID << std::endl;
	proc.display_allkids();






}

PCB *proc_tree::find_proc(int PID)
{
	if (PID == this->proc.PID) {
		return &this->proc;// do sprawdzenia czy tak zadziaa

	}
	else return this->proc.GET_kid(PID);

}

PCB * proc_tree::find_proc(const std::string& nazwa)
{
	{
		if (nazwa == this->proc.process_name) {
			return &this->proc;// do sprawdzenia czy tak zadziaa

		}
		else return this->proc.GET_kid(nazwa);

	}
}

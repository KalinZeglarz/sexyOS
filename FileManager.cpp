/**
	SexyOS
	FileManager.cpp
	Przeznaczenie: Zawiera definicje metod i konstruktor�w dla klas z FileManager.h

	@author Tomasz Kilja�czyk
	@version 02/11/18
*/

#include "FileManager.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <utility>

//--------------------------- Aliasy ------------------------
using u_int = unsigned int;

//------------------------- Operatory -----------------------
std::ostream& operator << (std::ostream& os, const std::vector<std::string>& strVec) {
	for (const std::string& str : strVec) {
		os << str << '\n';
	}
	return os;
}
std::ostream& operator << (std::ostream& os, const tm& time) {
	os << time.tm_hour << ':' << std::setfill('0') << std::setw(2) << time.tm_min << ' ' << std::setfill('0') << std::setw(2) << time.tm_mday << '.'
		<< std::setfill('0') << std::setw(2) << time.tm_mon << '.' << time.tm_year;
	return os;
}
bool operator == (const tm& time1, const tm& time2) {
	if (time1.tm_hour == time2.tm_hour  &&
		time1.tm_isdst == time2.tm_isdst &&
		time1.tm_mday == time2.tm_mday  &&
		time1.tm_min == time2.tm_min   &&
		time1.tm_mon == time2.tm_mon   &&
		time1.tm_sec == time2.tm_sec   &&
		time1.tm_wday == time2.tm_wday  &&
		time1.tm_yday == time2.tm_yday  &&
		time1.tm_year == time2.tm_year) {
		return true;
	}
	else { return false; };
}



//--------------------------- Dysk --------------------------

FileManager::FileSystem::FileSystem() {
	//Zape�nienie tablicy i-w�z��w pustymi i-w�z�ami
	for (u_int i = 0; i < INODE_NUMBER_LIMIT; i++) {
		inodeTable[i] = Inode();
	}
	//Wyzerowanie tablicy 'zaj�to�ci' i-w�z��w
	inodeBitVector.reset();
	for (u_int i = 0; i < bitVector.size(); i++) {
		bitVector[i] = BLOCK_FREE;
	}
}

const unsigned FileManager::FileSystem::get_free_inode_id() {
	for (u_int i = 0; i < inodeBitVector.size(); i++) {
		if (inodeBitVector[i] == false) { return i; }
	}
	return -1;
}

void FileManager::FileSystem::reset() {
	//Zape�nienie tablicy i-w�z��w pustymi i-w�z�ami
	for (u_int i = 0; i < INODE_NUMBER_LIMIT; i++) {
		inodeTable[i] = Inode();
	}
	//Wyzerowanie tablicy 'zaj�to�ci' i-w�z��w
	inodeBitVector.reset();
	for (u_int i = 0; i < bitVector.size(); i++) {
		bitVector[i] = BLOCK_FREE;
	}
}

FileManager::Inode::Inode() : blocksOccupied(0), realSize(0), creationTime(), modificationTime() {
	//Wype�nienie indeks�w blok�w dyskowych warto�ci� -1 (pusty indeks)
	directBlocks.fill(-1);
	for (u_int i = 0; i < BLOCK_INDEX_NUMBER; i++) { singleIndirectBlocks[i] = -1; }
}

void FileManager::Inode::clear() {
	blocksOccupied = 0;
	realSize = 0;
	directBlocks.fill(-1);
	singleIndirectBlocks.clear();
	creationTime = tm();
	modificationTime = tm();
	flagOpen = false;
}

FileManager::Disk::Disk() {
	//Zape�nanie naszego dysku zerowymi bajtami (symbolizuje pusty dysk)
	fill(space.begin(), space.end(), NULL);
}

void FileManager::IndexBlock::clear() {
	value.fill(-1);
}

u_int& FileManager::IndexBlock::operator[](const size_t& index) {
	return this->value[index];
}

const u_int& FileManager::IndexBlock::operator[](const size_t& index) const {
	return this->value[index];
}

void FileManager::Disk::write(const u_int& begin, const std::string& data) {
	const u_int end = begin + FileManager::BLOCK_SIZE - 1;
	//Indeks kt�ry b�dzie s�u�y� do wskazywania na kom�rki pami�ci
	u_int index = begin;
	//Iterowanie po danych typu string i zapisywanie znak�w na dysku
	for (u_int i = 0; i < data.size() && i <= end - begin; i++) {
		space[index] = data[i];
		index++;
	}
	//Zapisywanie NULL, je�li dane nie wype�ni�y ostatniego bloku
	for (; index <= end; index++) {
		space[index] = NULL;
	}
}

template<typename T>
const T FileManager::Disk::read(const u_int& begin) const {
	const u_int end = begin + FileManager::BLOCK_SIZE;
	T data;
	if (typeid(T) == typeid(std::string)) {
		//Odczytaj przestrze� dyskow� od indeksu begin do indeksu end
		for (u_int index = begin; index <= end; index++) {
			//Dodaj znak zapisany na dysku do danych
			data += space[index];
		}
	}
	return data;
}



//----------------------- FileManager -----------------------

FileManager::FileManager() {}



//-------------------- Podstawowe Metody --------------------

bool FileManager::file_create(const std::string& name) {
	try {
		if (name.empty()) { throw "Pusta nazwa!"; }
		std::vector<std::string> errorDescriptions;
		bool error = false;
		//Error1
		if (name.empty()) { errorDescriptions.emplace_back("Pusta nazwa!"); throw errorDescriptions; }
		//Error2
		if (FileSystem.rootDirectory.size() >= INODE_NUMBER_LIMIT) {
			errorDescriptions.emplace_back(
				"Wykorzystano wszystkie i-w�z�y!"); error = true;
		}
		//Error3
		if (check_if_name_used(name)) { errorDescriptions.push_back("Nazwa '" + name + "' jest ju� zaj�ta!"); error = true; }
		//Error4
		if (name.size() > MAX_FILENAME_LENGTH) { errorDescriptions.emplace_back("Nazwa za d�uga!"); error = true; }
		if (error) { throw errorDescriptions; }

		const u_int inodeId = FileSystem.get_free_inode_id();

		//Dodanie pliku do katalogu g��wnego
		FileSystem.rootDirectory[name] = inodeId;
		FileSystem.inodeBitVector[inodeId] = true;
		FileSystem.inodeTable[inodeId].creationTime = get_current_time_and_date();
		FileSystem.inodeTable[inodeId].modificationTime = get_current_time_and_date();

		if (messages) { std::cout << "Stworzono plik o nazwie '" << name << ".\n"; }
		return true;
	}
	catch (const std::string& description) {
		std::cout << description << '\n';
		return false;
	}
	catch (const std::vector<std::string>& descriptions) {
		std::cout << descriptions;
		return false;
	}
}

bool FileManager::file_write(const std::string& name, const std::string& data) {
	try {
		if (name.empty()) { throw "Pusta nazwa!"; }
		std::vector<std::string> errorDescriptions;
		bool error = false;
		//Error1
		if (data.size() > MAX_FILE_SIZE) { errorDescriptions.emplace_back("Podane dane przekraczaj� maksymalny rozmiar pliku!"); error = true; }

		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = FileSystem.rootDirectory.find(name);
		//Error2
		if (fileIterator == FileSystem.rootDirectory.end()) {
			errorDescriptions.push_back("Plik o nazwie '" + name + "' nie znaleziony.");
			throw errorDescriptions;
		}
		Inode* file = &FileSystem.inodeTable[fileIterator->second];

		//Error3
		if (data.size() > FileSystem.freeSpace - file->blocksOccupied*BLOCK_SIZE) {
			errorDescriptions.
				emplace_back("Za ma�o miejsca na dysku!"); error = true;
		}
		if (error) { throw errorDescriptions; }

		file_write(file, data);
		if (messages) { std::cout << "Zapisano dane do pliku o nazwie '" << name << "'.\n"; }
		return true;
	}
	catch (const std::string& description) {
		std::cout << description << '\n';
		return false;
	}
	catch (const std::vector<std::string>& descriptions) {
		std::cout << descriptions;
		return false;
	}
}

const std::string FileManager::file_read_all(const std::string& name) {
	try {
		if (name.empty()) { throw "Pusta nazwa!"; }
		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = FileSystem.rootDirectory.find(name);

		//Error1
		if (fileIterator == FileSystem.rootDirectory.end()) {
			throw("Plik o nazwie '" + name + "' nie znaleziony!");
		}
		Inode* inode = &FileSystem.inodeTable[fileIterator->second];

		return file_read_all(inode);
	}
	catch (const std::string& description) {
		std::cout << description << '\n';
		return "";
	}
}

bool FileManager::file_delete(const std::string& name) {
	try {
		if (name.empty()) { throw "Pusta nazwa!"; }
		//Iterator zwracany podczas przeszukiwania katalogu g��wnego za plikiem o podanej nazwie
		const auto fileIterator = FileSystem.rootDirectory.find(name);
		//Error1
		if (fileIterator == FileSystem.rootDirectory.end()) {
			throw("Plik o nazwie '" + name + "' nie znaleziony!");
		}

		Inode* inode = &FileSystem.inodeTable[fileIterator->second];

		file_deallocate(inode);
		FileSystem.inodeTable[fileIterator->second].clear();

		//Usu� wpis o pliku z obecnego katalogu
		FileSystem.rootDirectory.erase(fileIterator);

		if (messages) { std::cout << "Usuni�to plik o nazwie '" << name << "'.\n"; }
		return true;
	}
	catch (const std::string& description) {
		std::cout << description << '\n';
		return false;
	}
}

bool FileManager::file_open(const std::string & name) {
	try {
		//Error1
		if (name.empty()) { throw "Pusta nazwa!"; }
		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = FileSystem.rootDirectory.find(name);
		//Error2
		if (fileIterator == FileSystem.rootDirectory.end()) {
			throw("Plik o nazwie '" + name + "' nie znaleziony!");
		}

		Inode* inode = &FileSystem.inodeTable[fileIterator->second];

		//Error3
		if (inode->flagOpen == true) { throw ("Plik o nazwie '" + name + "' jest ju� otwarty!"); }

		inode->flagOpen = true; //Ustawia flag� plik otwarty

		if (messages) { std::cout << "Otwarto plik o nazwie '" << name << "'.\n"; }
		return true;
	}
	catch (const std::string& description) {
		std::cout << description << '\n';
		return false;
	}
}

bool FileManager::file_close(const std::string & name) {
	try {
		//Error1
		if (name.empty()) { throw "Pusta nazwa!"; }

		const auto fileIterator = FileSystem.rootDirectory.find(name);
		//Error2
		if (fileIterator == FileSystem.rootDirectory.end()) { throw("Plik o nazwie '" + name + "' nie znaleziony!"); }

		FileSystem.inodeTable[fileIterator->second].flagOpen = false; //Zeruje flag� plik otwarty
		usedFiles.erase(name);

		if (messages) { std::cout << "Zamkni�to plik o �cie�ce '" << name << "'.\n"; }
		return true;
	}
	catch (const std::string& description) {
		std::cout << description << '\n';
		return false;
	}
}



//--------------------- Dodatkowe metody --------------------
bool FileManager::disk_format() {
	try {
		if (!usedFiles.empty()) { throw "Nie mo�na sformatowa� dysku gdy pliki s� u�ywane!"; }

		FileSystem.reset();

		if (messages) { std::cout << "Sformatowano dysk!\n"; }
		return true;
	}
	catch (const std::string& description) {
		std::cout << description << '\n';
		return false;
	}
}

bool FileManager::file_create(const std::string& name, const std::string& data) {
	if (!file_create(name)) { return false; }
	//Zapisanie danych pliku na dysku
	return file_write(name, data);
}

bool FileManager::file_rename(const std::string& name, const std::string& changeName) {
	try {
		std::vector<std::string> errorDescriptions;
		bool error = false;

		//Error1
		if (name.empty()) { errorDescriptions.emplace_back("Pusta nazwa!"); throw errorDescriptions; }
		//Error2
		if (changeName.empty()) { errorDescriptions.emplace_back("Pusta nowa nazwa!"); throw errorDescriptions; }
		//Error3
		if (check_if_name_used(changeName)) {
			errorDescriptions.push_back("Nazwa '" + name + "' jest ju� zaj�ta!"); error = true;
		}
		//Error4
		if (changeName.size() > MAX_FILENAME_LENGTH) {
			errorDescriptions.emplace_back("Nowa nazwa za d�uga!"); error = true;
		}

		const auto fileIterator = FileSystem.rootDirectory.find(name);
		//Error5
		if (fileIterator == FileSystem.rootDirectory.end()) {
			errorDescriptions.push_back("Plik o nazwie '" + name + "' nie znaleziony!");
			throw errorDescriptions;
		}
		if (error) { throw errorDescriptions; }

		Inode* file = &FileSystem.inodeTable[fileIterator->second];

		//Lokowanie nowego klucza w tablicy hashowej katalogu g��wnego i przypisanie do niego id i-w�z�a
		FileSystem.rootDirectory[changeName] = fileIterator->second;

		//Usuni�cie starego klucza z katalogu g��wnego
		FileSystem.rootDirectory.erase(fileIterator);

		if (messages) { std::cout << "Zmieniono nazw� pliku '" << name << "' na '" << changeName << "'.\n"; }

		return true;
	}
	catch (const std::vector<std::string>& descriptions) {
		std::cout << descriptions;
		return false;
	}
}

void FileManager::set_messages(const bool& onOff) {
	messages = onOff;
}

void FileManager::set_detailed_messages(const bool&  onOff) {
	detailedMessages = onOff;
}


//------------------ Metody do wy�wietlania -----------------

void FileManager::display_file_system_params() const {
	std::cout << "Disk capacity: " << DISK_CAPACITY << " Bytes\n";
	std::cout << "Block size: " << BLOCK_SIZE << " Bytes\n";
	std::cout << "Max file size: " << MAX_FILE_SIZE << " Bytes\n";
	std::cout << "Max indexes in indirect index block: " << BLOCK_INDEX_NUMBER << " Indexes\n";
	std::cout << "Max direct indexes in file: " << BLOCK_INDEX_NUMBER << " Indexes\n";
	std::cout << "Max file number: " << INODE_NUMBER_LIMIT << " Files\n";
	std::cout << "Max filename length: " << MAX_FILENAME_LENGTH << " Characters\n";
}

void FileManager::display_root_directory_info() {
	std::cout << "Name: " << "root" << '\n';
	std::cout << "Size: " << calculate_directory_size() << " Bytes\n";
	std::cout << "Size on disk: " << calculate_directory_size_on_disk() << " Bytes\n";
	std::cout << "Contains: " << FileSystem.rootDirectory.size() << " Files\n";
}

bool FileManager::display_file_info(const std::string& name) {
	try {
		const auto fileIterator = FileSystem.rootDirectory.find(name);

		//Error1
		if (fileIterator == FileSystem.rootDirectory.end()) {
			throw("Plik o nazwie '" + name + "' nie znaleziony!");
		}

		const auto file = &FileSystem.inodeTable[fileIterator->second];

		std::cout << "Name: " << name << '\n';
		std::cout << "Size: " << file->realSize << " Bytes\n";
		std::cout << "Size on disk: " << file->blocksOccupied*BLOCK_SIZE << " Bytes\n";
		std::cout << "Created: " << file->creationTime << '\n';
		std::cout << "Modified: " << file->modificationTime << '\n';
		std::cout << "Saved data: " << file_read_all(file) << '\n';
		std::cout << "Direct block indexes: ";
		for (const auto& elem : file->directBlocks) {
			if (elem != -1) {
				std::cout << elem << ' ';
			}
			else { std::cout << -1 << ' '; }
		}
		std::cout << '\n';
		std::cout << "Indirect block indexes: ";
		for (u_int i = 0; i < BLOCK_INDEX_NUMBER; i++) {
			if (file->singleIndirectBlocks[i] != -1) {
				std::cout << file->singleIndirectBlocks[i] << ' ';
			}
			else { std::cout << -1 << ' '; }
		}
		std::cout << '\n';
		return true;
	}
	catch (const std::string& description) {
		std::cout << description << '\n';
		return false;
	}
}

void FileManager::display_root_directory() {
	std::cout << std::string(1, ' ') << "root/" << "\n";

	for (auto i = FileSystem.rootDirectory.begin(); i != FileSystem.rootDirectory.end(); ++i) {
		std::cout << std::string(2, ' ') << "- " << i->first << '\n';
	}
}

void FileManager::display_disk_content_binary() {
	u_int index = 0;
	for (const char& c : DISK.space) {
		std::cout << std::bitset<8>(c) << (index % BLOCK_SIZE == BLOCK_SIZE - 1 ? " , " : "") << (index % 16 == 15 ? " \n" : " ");
		index++;
	}
	std::cout << '\n';
}

void FileManager::display_disk_content_char() {
	u_int index = 0;
	for (const char& c : DISK.space) {
		if (c == ' ') { std::cout << ' '; }
		else if (c >= 0 && c <= 32) std::cout << ".";
		else std::cout << c;
		std::cout << (index % BLOCK_SIZE == BLOCK_SIZE - 1 ? " , " : "") << (index % 32 == 31 ? " \n" : " ");
		index++;
	}
	std::cout << '\n';
}

void FileManager::display_bit_vector() {
	u_int index = 0;
	for (u_int i = 0; i < FileSystem.bitVector.size(); i++) {
		if (i % 8 == 0) { std::cout << std::setfill('0') << std::setw(2) << (index / 8) + 1 << ". "; }
		std::cout << FileSystem.bitVector[i] << (index % 8 == 7 ? "\n" : " ");
		index++;
	}
	std::cout << '\n';
}



//------------------- Metody Sprawdzaj�ce -------------------

const bool FileManager::check_if_name_used(const std::string& name) {
	//Przeszukuje podany katalog za plikiem o tej samej nazwie
	if (FileSystem.rootDirectory.find(name) != FileSystem.rootDirectory.end()) {
		return true;
	}
	else { return false; }
}

const bool FileManager::check_if_enough_space(const u_int& dataSize) const {
	if (dataSize <= FileSystem.freeSpace) { return true; }
	return false;
}



//-------------------- Metody Obliczaj�ce -------------------

const u_int FileManager::calculate_needed_blocks(const size_t& dataSize) const {
	/*
	Przybli�enie w g�r� rozmiaru pliku przez rozmiar bloku.
	Jest tak, poniewa�, je�li zape�nia chocia� o jeden bajt
	wi�cej przy zaj�tym bloku, to trzeba zaalokowa� wtedy kolejny blok.
	*/
	return int(ceil(double(dataSize) / double(BLOCK_SIZE)));
}

const size_t FileManager::calculate_directory_size_on_disk() {
	//Rozmiar katalogu
	size_t size = 0;

	//Dodaje rozmiar plik�w w katalogu do rozmiaru katalogu
	for (const auto& element : FileSystem.rootDirectory) {
		size += FileSystem.inodeTable[element.second].blocksOccupied*BLOCK_SIZE;
	}
	return size;
}

const size_t FileManager::calculate_directory_size() {
	//Rzeczywisty rozmiar katalogu
	size_t realSize = 9;

	//Dodaje rzeczywisty rozmiar plik�w w katalogu do rozmiaru katalogu
	for (const auto& element : FileSystem.rootDirectory) {
		realSize += FileSystem.inodeTable[element.second].realSize;
	}
	return realSize;
}



//--------------------- Metody Alokacji ---------------------

void FileManager::file_truncate(Inode* file, const u_int& neededBlocks) {
	if (neededBlocks != file->blocksOccupied) {
		//Je�li nale�y zmniejszy� plik
		if (neededBlocks < file->blocksOccupied) { file_allocation_decrease(file, neededBlocks); }
		//Je�li nale�y zwi�kszy� plik
		else if (neededBlocks > file->blocksOccupied) { file_allocation_increase(file, neededBlocks); }
	}
}

void FileManager::file_add_indexes(Inode* file, const std::vector<u_int>& blocks) const {
	if (file != nullptr) {
		if (blocks.size() != size_t(0) && file->blocksOccupied * BLOCK_SIZE <= MAX_FILE_SIZE) {

			u_int blocksIndex = 0;
			//Wpisanie blok�w do bezpo�redniego bloku indeksowego
			for (size_t i = 0; i < BLOCK_INDEX_NUMBER && i < blocks.size(); i++) {
				file->directBlocks[i] = blocks[blocksIndex];
				blocksIndex++;
			}

			//Wpisanie blok�w do 1-poziomowego bloku indeksowego
			for (size_t i = 0; i < BLOCK_INDEX_NUMBER && blocksIndex < blocks.size(); i++) {
				file->singleIndirectBlocks[i] = blocks[blocksIndex];
				blocksIndex++;
			}
		}
	}
}

void FileManager::file_allocation_increase(Inode* file, const u_int& neededBlocks) {
	const u_int increaseBlocksNumber = abs(int(neededBlocks - file->blocksOccupied));
	bool fitsAfterLastIndex = true;
	u_int index = 0;
	u_int lastBlockIndex = 0;

	if (file->blocksOccupied != 0) {
		//Je�li indeksy zapisane s� tylko w bloku ideksowym 
		if (file->blocksOccupied <= BLOCK_INDEX_NUMBER) {
			for (u_int i = 0; i >= 0 && index == -1; i--) {
				index = file->directBlocks[i];
				if (i == 0) { break; } //u_int po obni�eniu zera przyjmuje warto�� wi�ksz� od zera
			}
		}
		else {
			for (u_int i = 0; i < BLOCK_INDEX_NUMBER && index != -1; i++) {
				lastBlockIndex = index;
				index = file->singleIndirectBlocks[i];
			}
		}

		for (u_int i = lastBlockIndex + 1; i < lastBlockIndex + increaseBlocksNumber + 1; i++) {
			if (FileSystem.bitVector[i] == BLOCK_OCCUPIED) { fitsAfterLastIndex = false; break; }
			else if (i == increaseBlocksNumber) { break; }
		}
		lastBlockIndex++;
	}
	else {
		fitsAfterLastIndex = false;
	}

	if (fitsAfterLastIndex) {
		std::vector<u_int>blocks;
		for (u_int i = lastBlockIndex; i < lastBlockIndex + increaseBlocksNumber; i++) {
			blocks.push_back(i);
		}
		file_allocate_blocks(file, blocks);
	}
	else {
		if (file->blocksOccupied > 0) { file_deallocate(file); };
		file_allocate_blocks(file, find_unallocated_blocks(neededBlocks));
	}
	if (detailedMessages) { std::cout << "Zwi�kszono plik do rozmiaru " << file->blocksOccupied*BLOCK_SIZE << " Bajt.\n"; }
}

void FileManager::file_allocation_decrease(Inode* file, const u_int& neededBlocks) {
	const u_int sizeToStart = neededBlocks * BLOCK_SIZE;
	//Zmienna do analizowania, czy ju� mo�na usuwa�/dodawa� bloki do pliku
	u_int currentSize = 0;

	u_int indexNumber = 0;
	u_int index = file->directBlocks[indexNumber];

	//Dop�ki indeks na co� wskazuje
	bool indirect = false;
	while (index != -1) {
		currentSize += BLOCK_SIZE;
		//Sprawdzenie czy mamy doczynienia z niebezpo�rednim blokiem indeksowym
		if (indexNumber == BLOCK_INDEX_NUMBER) {
			indirect = true;
			indexNumber = 0;
		}
		if (!indirect) {
			index = file->directBlocks[indexNumber];
		}
		else if (indirect) {
			index = file->singleIndirectBlocks[indexNumber];
		}
		//Spisz kolejny indeks
		indexNumber++;

		//Je�li obecny rozmiar przewy�sza rozmiar potrzebny do rozpocz�cia usuwania
		//zacznij usuwa� bloki
		if (currentSize > sizeToStart && index != -1) {
			//Zmniejszenie rozmiaru pliku
			file->blocksOccupied--;
			//Po uci�ciu rozmiar i rozmiar rzeczywisty b�d� takie same
			file->realSize = file->blocksOccupied*BLOCK_SIZE;
			//Oznacz obecny indeks jako wolny
			change_bit_vector_value(index, BLOCK_FREE);
			//Obecny indeks w tablicy FileSystem wskazuje na nic
			if (!indirect) {
				file->directBlocks[index] = -1;
			}
			else if (indirect) {
				file->singleIndirectBlocks[index] = -1;
			}
		}
	}
	if (detailedMessages) { std::cout << "Zmniejszono plik do rozmiaru " << file->blocksOccupied*BLOCK_SIZE << " Bajt.\n"; }
}

void FileManager::file_deallocate(Inode* file) {
	std::vector<u_int>freedBlocks; //Zmienna u�yta do wy�wietlenia komunikatu

	if (file->directBlocks[0] != -1) {
		u_int index = 0;
		u_int indexNumber = 0;
		bool indirect = false;

		while (index != -1 && indexNumber < MAX_FILE_SIZE / BLOCK_SIZE) {
			if (indexNumber == BLOCK_INDEX_NUMBER) {
				indirect = true;
				indexNumber = 0;
			}
			if (!indirect) {
				index = file->directBlocks[indexNumber];
				file->directBlocks[indexNumber] = -1;
			}
			else if (indirect) {
				index = file->singleIndirectBlocks[indexNumber];
				file->singleIndirectBlocks[indexNumber] = -1;
			}
			else { index = -1; }

			if (index != -1) {
				freedBlocks.push_back(index);
				change_bit_vector_value(index, BLOCK_FREE);
				indexNumber++;
			}
		}
		file->directBlocks.fill(-1);
		file->blocksOccupied = 0;
	}
	if (detailedMessages) {
		std::sort(freedBlocks.begin(), freedBlocks.end());
		std::cout << "Zwolniono bloki: ";
		for (u_int i = 0; i < freedBlocks.size(); i++) { std::cout << freedBlocks[i] << (i < freedBlocks.size() - 1 ? ", " : ""); }
		std::cout << '\n';
	}
}

void FileManager::file_allocate_blocks(Inode* file, const std::vector<u_int>& blocks) {

	for (const auto& i : blocks) {
		change_bit_vector_value(i, BLOCK_OCCUPIED);
	}
	file->blocksOccupied += blocks.size();

	file_add_indexes(file, blocks);

	if (detailedMessages) {
		std::cout << "Zaalokowano bloki: ";
		for (u_int i = 0; i < blocks.size(); i++) { std::cout << blocks[i] << (i < blocks.size() - 1 ? ", " : ".\n"); }
	}
}

const std::vector<u_int> FileManager::find_unallocated_blocks_fragmented(u_int blockNumber) {
	//Lista wolnych blok�w
	std::vector<u_int> blockList;

	//Szuka wolnych blok�w
	for (u_int i = 0; i < FileSystem.bitVector.size(); i++) {
		//Je�li blok wolny
		if (FileSystem.bitVector[i] == BLOCK_FREE) {
			//Dodaje indeks bloku
			blockList.push_back(i);
			//Potrzeba teraz jeden blok mniej
			blockNumber--;
			//Je�li potrzeba 0 blok�w, przerwij
			if (blockNumber == 0) { break; }
		}
	}
	return blockList;
}

const std::vector<u_int> FileManager::find_unallocated_blocks_best_fit(const u_int& blockNumber) {
	//Lista indeks�w blok�w (dopasowanie)
	std::vector<u_int> blockList;
	//Najlepsze dopasowanie
	std::vector<u_int> bestBlockList(FileSystem.bitVector.size() + 1);

	//Szukanie wolnych blok�w spe�niaj�cych minimum miejsca
	for (u_int i = 0; i < FileSystem.bitVector.size(); i++) {
		if (FileSystem.bitVector[i] == BLOCK_FREE) {
			//Dodaj indeks bloku do listy blok�w
			blockList.push_back(i);
		}
		else {
			//Je�li uzyskana lista blok�w jest wi�ksza od ilo�ci blok�w jak� chcemy uzyska�
			//to dodaj uzyskane dopasowanie do listy dopasowa�;
			if (blockList.size() >= blockNumber) {
				//Je�li znalezione dopasowanie mniejsze ni� najlepsze dopasowanie
				if (blockList.size() < bestBlockList.size()) {
					bestBlockList = blockList;
					if (bestBlockList.size() == blockNumber) { break; }
				}
			}

			blockList.clear();
		}
	}

	/*
	Je�li zdarzy si�, �e ostatni blok w wektorze bitowym jest wolny, to
	ostatnie dopasownie nie zostanie dodane do listy dopasowa�, dlatego
	trzeba wykona� poni�szy kod. Je�li ostatni blok w wektorze bitowym
	b�dzie zaj�ty to blockList b�dzie pusty i nie spie�ni warunku
	*/
	if (blockList.size() >= blockNumber) {
		if (blockList.size() < bestBlockList.size()) {
			bestBlockList = blockList;
		}
	}

	//Je�li znalezione najlepsze dopasowanie
	if (bestBlockList.size() < FileSystem.bitVector.size()) {
		//Odetnij nadmiarowe indeksy z dopasowania (je�li wi�ksze ni� potrzeba)
		bestBlockList.resize(blockNumber);
	}
	else { bestBlockList.resize(0); }

	return bestBlockList;
}

const std::vector<u_int> FileManager::find_unallocated_blocks(const u_int& blockNumber) {
	//Szuka blok�w funkcj� z metod� best-fit
	std::vector<u_int> blockList = find_unallocated_blocks_best_fit(blockNumber);

	//Je�li funkcja z metod� best-fit nie znajdzie dopasowa�
	if (blockList.empty()) {
		//Szuka niezaalokowanych blok�w, wybieraj�c pierwsze wolne
		blockList = find_unallocated_blocks_fragmented(blockNumber);
	}

	return blockList;
}



//----------------------- Metody Inne -----------------------

void FileManager::file_write(Inode* file, const std::string& data) {
	file->modificationTime = get_current_time_and_date();

	//Uzyskuje dane podzielone na fragmenty
	const std::vector<std::string>fileFragments = data_to_data_fragments(data);

	//Alokowanie blok�w dla pliku
	file_truncate(file, fileFragments.size());

	file->realSize = data.size();

	//Indeks pod kt�rym maj� zapisywane by� dane
	u_int indexNumber = 0;
	u_int index = file->directBlocks[0];
	bool indirect = false;

	//Zapisuje wszystkie dane na dysku
	for (const auto& fileFragment : fileFragments) {
		//Zapisuje fragment na dysku
		DISK.write(index * BLOCK_SIZE, fileFragment);
		//Przypisuje do indeksu numer kolejnego bloku
		indexNumber++;
		if (indexNumber == BLOCK_INDEX_NUMBER) {
			indirect = true;
			indexNumber = 0;
		}
		if (!indirect) {
			index = file->directBlocks[indexNumber];
		}
		else if (indirect) {
			index = file->singleIndirectBlocks[indexNumber];
		}
	}
}

const std::string FileManager::file_read_all(Inode* file) const {
	std::string data;
	//Indeks do wczytywania danych z dysku
	size_t indexNumber = 0;
	u_int index = 0;

	//Dop�ki nie natrafimy na koniec danych pliku
	bool indirect = false;
	while (index != -1) {
		if (indexNumber == BLOCK_INDEX_NUMBER && indirect) { break; }
		else if (indexNumber == BLOCK_INDEX_NUMBER) {
			indirect = true;
			indexNumber = 0;
		}

		if (!indirect) {
			index = file->directBlocks[indexNumber];
		}
		else if (indirect) {
			index = file->singleIndirectBlocks[indexNumber];
		}

		//Dodaje do danych fragment pliku pod wskazanym indeksem
		data += DISK.read<std::string>(index * BLOCK_SIZE);
		//Zwi�ksza indeks iteruj�cy po blokach indeksowych
		indexNumber++;
	}
	return data;
}

const tm FileManager::get_current_time_and_date() {
	time_t tt;
	time(&tt);
	tm timeAndDate = *localtime(&tt);
	timeAndDate.tm_year += 1900;
	timeAndDate.tm_mon += 1;
	return timeAndDate;
}

void FileManager::change_bit_vector_value(const u_int& block, const bool& value) {
	//Je�li warto�� zaj�ty to wolne miejsce - BLOCK_SIZE
	if (value == 1) { FileSystem.freeSpace -= BLOCK_SIZE; }
	//Je�li warto�� wolny to wolne miejsce + BLOCK_SIZE
	else if (value == 0) { FileSystem.freeSpace += BLOCK_SIZE; }
	//Przypisanie blokowi podanej warto�ci
	FileSystem.bitVector[block] = value;
}

const std::vector<std::string> FileManager::data_to_data_fragments(const std::string& data) const {
	//Tablica fragment�w podanych danych
	std::vector<std::string>fileFragments;

	//Przetworzenie ca�ych danych
	for (u_int i = 0; i < calculate_needed_blocks(data.size()); i++) {
		//Oblicza pocz�tek kolejnej cz�ci fragmentu danych.
		const u_int substrBegin = i * BLOCK_SIZE;
		//Dodaje do tablicy fragment�w kolejny fragment o d�ugo�ci BLOCK_SIZE
		fileFragments.push_back(data.substr(substrBegin, BLOCK_SIZE));
	}
	return fileFragments;
}

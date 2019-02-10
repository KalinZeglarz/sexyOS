/**
	SexyOS
	FileManager.cpp
	Przeznaczenie: Zawiera definicje metod i konstruktor�w dla klas z FileManager.h

	@author Tomasz Kilja�czyk
*/

/*
 * Aby �atwiej nawigowa� po moim kodzie polecam z�o�y� wszystko
 * Skr�t: CTRL + M + A
 */

#include "FileManager.h"
#include "Planist.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace std;

//--------------------------- Aliasy ------------------------
using u_int = unsigned int;
using u_short_int = unsigned short int;
using u_char = unsigned char;



//----------------------- System Plik�w ---------------------

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

unsigned FileManager::FileSystem::get_free_inode_id() {
	for (u_int i = 0; i < inodeBitVector.size(); i++) {
		if (!inodeBitVector[i]) { return i; }
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



//-------------------------- I-w�ze� ------------------------

FileManager::Inode::Inode() : creationTime(), modificationTime() {
	//Wype�nienie indeks�w blok�w dyskowych warto�ci� -1 (pusty indeks)
	directBlocks.fill(-1);
	singleIndirectBlocks = -1;
}

void FileManager::Inode::clear() {
	blocksOccupied = 0;
	realSize = 0;
	directBlocks.fill(-1);
	singleIndirectBlocks = -1;
	creationTime = tm();
	modificationTime = tm();
	//flagOpen = false;
}



//--------------------------- Dysk --------------------------

FileManager::Disk::Disk() {
	//Zape�nanie naszego dysku zerowymi bajtami (symbolizuje pusty dysk)
	space.fill(0);
}

void FileManager::Disk::write(const u_short_int& begin, const string& data) {
	const u_int end = begin + BLOCK_SIZE - 1;

	//Indeks kt�ry b�dzie s�u�y� do wskazywania na kom�rki pami�ci
	u_int index = begin;

	//Iterowanie po danych typu string i zapisywanie znak�w na dysku
	for (u_int i = 0; i < data.size() && i <= end - begin; i++) {
		space[index] = data[i];
		index++;
	}
	//Zapisywanie 0, je�li dane nie wype�ni�y ostatniego bloku
	//for (; index <= end; index++) {
	//	space[index] = 0;
	//}
}

void FileManager::Disk::write(const u_short_int& begin, const array<u_int, BLOCK_SIZE / 2>& data) {
	string result;
	for (size_t i = 0; i < BLOCK_SIZE / 2; i++) {
		string temp;
		if (data[i] != -1) {
			if (data[i] <= 9) {
				temp = "0" + to_string(data[i]);
			}
			else { temp = to_string(data[i]); }
			result += temp;
		}
		else { result += "-1"; }
	}
	write(begin, result);
}

const string FileManager::Disk::read_str(const u_int& begin) const {
	string data;
	const u_int end = begin + BLOCK_SIZE;
	//Odczytaj przestrze� dyskow� od indeksu begin do indeksu end
	for (u_int index = begin; index < end; index++) {
		//Dodaj znak zapisany na dysku do danych
		if (space[index] != 0) { data += space[index]; }
	}
	return data;
}

const array<u_int, FileManager::BLOCK_SIZE / 2> FileManager::Disk::read_arr(const u_int& begin) const {
	const u_int end = begin + BLOCK_SIZE;		//Odczytywany jest jeden blok
	array<u_int, BLOCK_SIZE / 2> result{};	//Jedna liczba zajmuje 2 bajty
	string data;

	result.fill(-1);

	//Odczytaj przestrze� dyskow� od indeksu begin do indeksu end
	for (u_int index = begin; index < end; index++) {
		//Dodaj znak zapisany na dysku do danych
		if (space[index] != 0) { data += space[index]; }
	}

	string num;
	u_int arrIndex = 0;
	for (const char& c : data) {
		if (c == 0) { break; }
		num += c;
		if (num.length() == 2) {
			result[arrIndex] = stoi(num);
			num.clear();
			arrIndex++;
		}
		if (arrIndex == BLOCK_SIZE / 2) { break; }
	}
	return result;
}



//------------------------- File IO -------------------------

void FileManager::FileIO::buffer_update(const int8_t& blockNumber) {
	if (blockNumber < BLOCK_INDEX_NUMBER) {
		buffer = disk->read_str(file->directBlocks[blockNumber] * BLOCK_SIZE);
	}
	else if (file->singleIndirectBlocks != -1) {
		array<u_int, BLOCK_SIZE / 2>blocks{};
		blocks.fill(-1);
		blocks = disk->read_arr(file->singleIndirectBlocks*BLOCK_SIZE);

		buffer = disk->read_str(blocks[blockNumber - BLOCK_INDEX_NUMBER] * BLOCK_SIZE);
	}
	else { buffer = '\0'; }
}

string FileManager::FileIO::read(const u_short_int& byteNumber) {
	array<u_int, BLOCK_SIZE / 2>indirectBlocks{};

	indirectBlocks.fill(-1);

	auto blockIndex = static_cast<uint8_t>(floor(readPos / BLOCK_SIZE));
	if (blockIndex > BLOCK_INDEX_NUMBER + BLOCK_SIZE / 2) { return ""; }

	string data;
	u_short_int bytesRead = 0; //Zmienna zliczaj�ca odczytane bajty
	uint8_t blockIndexPrev = -1; //Zmienna u�ywana do sprawdzenia czy nale�y zaktualizowa� bufor

	while (bytesRead < byteNumber) {
		if (readPos >= file->realSize) { break; }
		if (blockIndex != blockIndexPrev) {
			this->buffer_update(blockIndex);
			blockIndexPrev = blockIndex;
		}

		data += this->buffer[readPos%BLOCK_SIZE];

		readPos++;
		bytesRead++;
		blockIndex = static_cast<int8_t>(floor(readPos / BLOCK_SIZE));
	}

	return data;
}

string FileManager::FileIO::read_all() {
	this->reset_read_pos();
	return read(file->realSize);
}

void FileManager::FileIO::write(const vector<string>& dataFragments, const int8_t& startIndex) const {
	u_int indexNumber = startIndex;
	u_int index;

	bool indirect = false;

	array<u_int, BLOCK_SIZE / 2>indirectBlocks{};
	indirectBlocks.fill(-1);
	if (file->singleIndirectBlocks != -1) {
		indirectBlocks = disk->read_arr(file->singleIndirectBlocks*BLOCK_SIZE);
	}

	if (startIndex < BLOCK_INDEX_NUMBER) { index = file->directBlocks[startIndex]; }
	else { index = indirectBlocks[startIndex - BLOCK_INDEX_NUMBER]; }

	//Zapisuje wszystkie dane na dysku
	for (const auto& fileFragment : dataFragments) {
		if (index == -1) { break; }

		//Zapisuje fragment na dysku
		disk->write(index * BLOCK_SIZE, fileFragment);

		//Przypisuje do indeksu numer kolejnego bloku
		indexNumber++;
		if (indexNumber == BLOCK_INDEX_NUMBER && !indirect) {
			indirect = true;
			indexNumber = 0;
		}
		if (!indirect) { index = file->directBlocks[indexNumber]; }
		else if (indirect) { index = indirectBlocks[indexNumber]; }
	}
}

const bitset<2> FileManager::FileIO::get_flags() const {
	bitset<2> flags;
	flags[READ_FLAG] = readFlag; flags[WRITE_FLAG] = writeFlag;
	return flags;
}



//------------------- Metody Sprawdzaj�ce -------------------

bool FileManager::check_if_name_used(const string& name) {
	//Przeszukuje podany katalog za plikiem o tej samej nazwie
	return fileSystem.rootDirectory.find(name) != fileSystem.rootDirectory.end();
}

bool FileManager::check_if_enough_space(const u_int& dataSize) const {
	return dataSize <= fileSystem.freeSpace;
}



//-------------------- Metody Obliczaj�ce -------------------

u_int FileManager::calculate_needed_blocks(const size_t& dataSize) {
	/*
	Przybli�enie w g�r� rozmiaru pliku przez rozmiar bloku.
	Jest tak, poniewa�, je�li zape�nia chocia� o jeden bajt
	wi�cej przy zaj�tym bloku, to trzeba zaalokowa� wtedy kolejny blok.
	*/
	return int(ceil(double(dataSize) / double(BLOCK_SIZE)));
}

size_t FileManager::calculate_directory_size_on_disk() {
	//Rozmiar katalogu
	size_t size = 0;

	//Dodaje rozmiar plik�w w katalogu do rozmiaru katalogu
	for (const auto& element : fileSystem.rootDirectory) {
		size += fileSystem.inodeTable[element.second].blocksOccupied*BLOCK_SIZE;
	}
	return size;
}

size_t FileManager::calculate_directory_size() {
	//Rzeczywisty rozmiar katalogu
	size_t realSize = 0;

	//Dodaje rzeczywisty rozmiar plik�w w katalogu do rozmiaru katalogu
	for (const auto& element : fileSystem.rootDirectory) {
		realSize += fileSystem.inodeTable[element.second].realSize;
	}
	return realSize;
}



//--------------------- Metody Alokacji ---------------------

void FileManager::file_truncate(Inode* file, const u_int& neededBlocks) {
	if (neededBlocks != file->blocksOccupied) {
		if (neededBlocks > file->blocksOccupied) { file_allocation_increase(file, neededBlocks); }
	}
}

void FileManager::file_add_indexes(Inode* file, const vector<u_int>& blocks) {
	if (file != nullptr) {
		if (!blocks.empty() && file->blocksOccupied * BLOCK_SIZE <= MAX_FILE_SIZE) {

			u_int blocksIndex = 0;
			//Wpisanie blok�w do bezpo�redniego bloku indeksowego
			for (size_t i = 0; i < BLOCK_INDEX_NUMBER && blocksIndex < blocks.size(); i++) {
				if (file->directBlocks[i] == -1) {
					file->directBlocks[i] = blocks[blocksIndex];
					blocksIndex++;
				}
			}

			//Wpisanie blok�w do 1-poziomowego bloku indeksowego
			if (file->blocksOccupied > BLOCK_INDEX_NUMBER) {
				const u_int index = find_unallocated_blocks(1)[0];

				file->blocksOccupied++;
				file->singleIndirectBlocks = index;

				array<u_int, BLOCK_SIZE / 2> indirectBlocks{};
				indirectBlocks.fill(-1);

				for (size_t i = 0; i < BLOCK_SIZE / 2 && blocksIndex < blocks.size(); i++) {
					if (indirectBlocks[i] == -1) {
						indirectBlocks[i] = blocks[blocksIndex];
						blocksIndex++;
					}
				}
				disk.write(index * BLOCK_SIZE, indirectBlocks);
				fileSystem.bitVector[index] = BLOCK_OCCUPIED;
				fileSystem.freeSpace -= BLOCK_SIZE;
				if (detailedMessages) { cout << "Stworzono blok indeksowy pod indeksem: " << index << '\n'; }
			}
		}
	}
}

void FileManager::file_allocation_increase(Inode* file, const u_int& neededBlocks) {
	const u_int increaseBlocksNumber = abs(int(neededBlocks - file->blocksOccupied));
	bool fitsAfterLastIndex = true;
	u_int index = -1;
	u_int lastBlockIndex = -1;

	if (file->blocksOccupied != 0) {
		//Je�li indeksy zapisane s� tylko w bezpo�rednim bloku indeksowym 
		if (file->blocksOccupied <= BLOCK_INDEX_NUMBER) {
			for (int i = BLOCK_INDEX_NUMBER - 1; i >= 0 && index == -1; i--) {
				index = file->directBlocks[i];
				lastBlockIndex = index;
			}
		}
		else {
			array<u_int, BLOCK_SIZE / 2>indirectBlocks{};
			indirectBlocks.fill(-1);
			indirectBlocks = disk.read_arr(file->singleIndirectBlocks*BLOCK_SIZE);

			for (int i = BLOCK_SIZE / 2 - 1; i >= 0 && index == -1; i--) {
				index = indirectBlocks[i];
				lastBlockIndex = index;
			}
		}
		for (u_int i = lastBlockIndex + 1; i < lastBlockIndex + increaseBlocksNumber + 1; i++) {
			if (fileSystem.bitVector[i] == BLOCK_OCCUPIED) { fitsAfterLastIndex = false; break; }
		}
		lastBlockIndex++;
	}
	else {
		fitsAfterLastIndex = false;
	}

	if (fitsAfterLastIndex) {
		vector<u_int>blocks;
		for (u_int i = lastBlockIndex; i < lastBlockIndex + increaseBlocksNumber; i++) {
			blocks.push_back(i);
		}
		file_allocate_blocks(file, blocks);
	}
	else {
		if (file->blocksOccupied > 0) { file_deallocate(file); }
		file_allocate_blocks(file, find_unallocated_blocks(neededBlocks));
	}
	if (detailedMessages) { cout << "Zwiekszono plik do rozmiaru " << file->blocksOccupied*BLOCK_SIZE << " Bajt.\n"; }
}

void FileManager::file_deallocate(Inode* file) {
	vector<u_int>freedBlocks; //Zmienna u�yta do wy�wietlenia komunikatu

	if (file->directBlocks[0] != -1) {
		u_int index = 0;
		u_int indexNumber = 0;
		bool indirect = false;

		array<u_int, BLOCK_SIZE / 2>indirectBlocks{};
		indirectBlocks.fill(-1);
		if (file->singleIndirectBlocks != -1) {
			indirectBlocks = disk.read_arr(file->singleIndirectBlocks*BLOCK_SIZE);
		}

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
				index = indirectBlocks[indexNumber];
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
		if (!freedBlocks.empty()) {
			sort(freedBlocks.begin(), freedBlocks.end());
			cout << "Zwolniono bloki: ";
			for (u_int i = 0; i < freedBlocks.size(); i++) { cout << freedBlocks[i] << (i < freedBlocks.size() - 1 ? ", " : ""); }
			cout << '\n';
		}
	}
}

void FileManager::file_allocate_blocks(Inode* file, const vector<u_int>& blocks) {

	for (const auto& i : blocks) {
		change_bit_vector_value(i, BLOCK_OCCUPIED);
	}
	file->blocksOccupied += static_cast<int8_t>(blocks.size());

	file_add_indexes(file, blocks);

	if (detailedMessages) {
		cout << "Zaalokowano bloki: ";
		for (u_int i = 0; i < blocks.size(); i++) { cout << blocks[i] << (i < blocks.size() - 1 ? ", " : ".\n"); }
	}
}

const vector<u_int> FileManager::find_unallocated_blocks_fragmented(u_int blockNumber) {
	//Lista wolnych blok�w
	vector<u_int> blockList;

	//Szuka wolnych blok�w
	for (u_int i = 0; i < fileSystem.bitVector.size(); i++) {
		//Je�li blok wolny
		if (fileSystem.bitVector[i] == BLOCK_FREE) {
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

const vector<u_int> FileManager::find_unallocated_blocks_best_fit(const u_int& blockNumber) {
	//Lista indeks�w blok�w (dopasowanie)
	vector<u_int> blockList;
	//Najlepsze dopasowanie
	vector<u_int> bestBlockList(fileSystem.bitVector.size() + 1);

	//Szukanie wolnych blok�w spe�niaj�cych minimum miejsca
	for (u_int i = 0; i < fileSystem.bitVector.size(); i++) {
		if (fileSystem.bitVector[i] == BLOCK_FREE) {
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
	if (bestBlockList.size() < fileSystem.bitVector.size()) {
		//Odetnij nadmiarowe indeksy z dopasowania (je�li wi�ksze ni� potrzeba)
		bestBlockList.resize(blockNumber);
	}
	else { bestBlockList.resize(0); }

	return bestBlockList;
}

const vector<u_int> FileManager::find_unallocated_blocks(const u_int& blockNumber) {
	//Szuka blok�w funkcj� z metod� best-fit
	vector<u_int> blockList = find_unallocated_blocks_best_fit(blockNumber);

	//Je�li funkcja z metod� best-fit nie znajdzie dopasowa�
	if (blockList.empty()) {
		//Szuka niezaalokowanych blok�w, wybieraj�c pierwsze wolne
		blockList = find_unallocated_blocks_fragmented(blockNumber);
	}

	return blockList;
}



//----------------------- Metody Inne -----------------------

bool FileManager::is_file_opened_write(const string& fileName) {
	for (auto& elem : accessedFiles) {
		if (elem.first.first == fileName) {
			if (elem.second.get_flags()[1]) { return true; }
		}
	}
	return false;
}

int FileManager::file_accessing_proc_count(const string& fileName) {
	int result = 0;
	for (auto& elem : accessedFiles) {
		if (elem.first.first == fileName) { result++; }
	}
	return result;
}

string FileManager::get_file_data_block(Inode* file, const int8_t& indexNumber) const {
	string data;

	if (indexNumber < BLOCK_INDEX_NUMBER) {
		data = disk.read_str(file->directBlocks[indexNumber] * BLOCK_SIZE);
	}
	else if (file->singleIndirectBlocks != -1) {
		array<u_int, BLOCK_SIZE / 2>blocks{};
		blocks.fill(-1);
		blocks = disk.read_arr(file->singleIndirectBlocks*BLOCK_SIZE);

		data = disk.read_str(blocks[indexNumber - BLOCK_INDEX_NUMBER]);
	}
	return data;
}

void FileManager::file_write(Inode* file, FileIO* IO, const string& data) {
	file->modificationTime = get_current_time_and_date();

	//Uzyskuje dane podzielone na fragmenty
	const vector<string>dataFragments = fragmentate_data(data);

	//Alokowanie blok�w dla pliku
	file_truncate(file, dataFragments.size());

	IO->write(dataFragments, 0);

	file->realSize = static_cast<u_short_int>(data.size());
}

void FileManager::file_append(Inode* file, FileIO* IO, const string& data) {
	file->modificationTime = get_current_time_and_date();
	string data_ = data;

	int8_t surplusBlock = 0;
	if (file->realSize % BLOCK_SIZE + data_.size() % BLOCK_SIZE > BLOCK_SIZE) { surplusBlock = 1; }

	auto lastBlockIndex = static_cast<int8_t>(floor(file->realSize / BLOCK_SIZE));
	if (file->realSize % BLOCK_SIZE != 0) {
		data_.insert(0, get_file_data_block(file, lastBlockIndex));
	}
	else { lastBlockIndex += surplusBlock; }

	const vector<string>dataFragments = fragmentate_data(data_);

	//Alokowanie blok�w dla nowych danych
	file->blocksOccupied += static_cast<int8_t>(dataFragments.size()) - surplusBlock;
	file_allocate_blocks(file, find_unallocated_blocks(dataFragments.size() - surplusBlock));

	IO->write(dataFragments, lastBlockIndex);

	file->realSize += static_cast<u_short_int>(data.size());
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
	if (value == 1) { fileSystem.freeSpace -= BLOCK_SIZE; }
	//Je�li warto�� wolny to wolne miejsce + BLOCK_SIZE
	else if (value == 0) { fileSystem.freeSpace += BLOCK_SIZE; }
	//Przypisanie blokowi podanej warto�ci
	fileSystem.bitVector[block] = value;
}

const vector<string> FileManager::fragmentate_data(const string& data) {
	//Tablica fragment�w podanych danych
	vector<string>fileFragments;

	//Przetworzenie ca�ych danych
	for (u_int i = 0; i < calculate_needed_blocks(data.size()); i++) {
		//Oblicza pocz�tek kolejnej cz�ci fragmentu danych.
		const u_int substrBegin = i * BLOCK_SIZE;
		//Dodaje do tablicy fragment�w kolejny fragment o d�ugo�ci BLOCK_SIZE
		fileFragments.push_back(data.substr(substrBegin, BLOCK_SIZE));
	}
	return fileFragments;
}

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

const std::string Serializer::IntToString(unsigned int input) {
	std::string result;

	while (input != 0)
	{
		if (input % 255 == 0)
		{
			result.push_back(char(255));
			input -= 255;
		}
		else
		{
			result.push_back(input % 255);
			input -= input % 255;
		}
	}

	return result;
}
const unsigned Serializer::StringToInt(const std::string& input) {
	unsigned int result = 0;
	for (const char& c : input)
	{
		result += std::bitset<8>(c).to_ulong();
	}
	return result;
}

using u_int = unsigned int;

//Operator do wy�wietlania czasu z dat�
std::ostream& operator << (std::ostream &os, const tm &time) {
	os << time.tm_hour << ':' << time.tm_min << ' ' << time.tm_mday << '.' << time.tm_mon << '.' << time.tm_year;
	return os;
}

//--------------------------- Dysk --------------------------

FileManager::Disk::Disk() {
	//Zape�nanie naszego dysku zerowymi bajtami (symbolizuje pusty dysk)
	fill(space.begin(), space.end(), NULL);
}

std::shared_ptr<FileManager::Index>& FileManager::IndexBlock::operator[](const size_t &index) {
	return this->value[index];
}

const std::shared_ptr<FileManager::Index>& FileManager::IndexBlock::operator[](const size_t &index) const {
	return this->value[index];
}

void FileManager::Disk::write(const u_int &begin, const u_int &end, const std::string &data) {
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

void FileManager::Disk::write(const u_int &index, const u_int &data) {
	//Zapisz liczb� pod danym indeksem
	space[index] = data;
}

template<typename T>
const T FileManager::Disk::read(const u_int &begin, const u_int &end) const {
	//Dane
	T data;

	//Je�li typ danych to string
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

FileManager::FileManager() {
	//Przypisanie katalogu g��wnego do obecnego katalogu 
	currentDirectory = std::dynamic_pointer_cast<Directory>(DISK.FileSystem.iNodeTable[0]);
}

//-------------------- Podstawowe Metody --------------------

void FileManager::FileCreate(const std::string &name, const std::string &data) {
	//Rozmiar pliku obliczony na podstawie podanych danych
	const u_int fileSize = CalculateNeededBlocks(data)*BLOCK_SIZE;

	if (fileNumber < TOTAL_FILE_NUMBER_LIMIT) {
		//Je�li plik si� zmie�ci i nazwa nie u�yta
		if (CheckIfEnoughSpace(fileSize) && data.size() <= MAX_FILE_SIZE && CheckIfNameUnused(currentDirectory, name)) {
			//Je�li �cie�ka nie przekracza maksymalnej d�ugo�ci
			if (name.size() + GetCurrentPathLength() < MAX_PATH_LENGTH) {
				fileNumber++;
				std::shared_ptr<File> file = std::make_shared<File>();
				file->blocksOccupied = fileSize / BLOCK_SIZE;
				file->sizeOnDisk = data.size();

				//Zapisywanie daty stworzenia pliku
				file->creationTime = GetCurrentTimeAndDate();
				file->modificationTime = file->creationTime;

				//Lista indeks�w blok�w, kt�re zostan� zaalokowane na potrzeby pliku
				const std::vector<u_int> blocks = FindUnallocatedBlocks(file->blocksOccupied);

				//Wpisanie blok�w do bezpo�redniego bloku indeksowego
				for (size_t i = 0; i < BLOCK_DIRECT_INDEX && i < blocks.size(); i++) {
					file->directBlocks[i] = std::make_shared<Index>(blocks[i]);
				}
				//Wpisanie blok�w do 1-poziomowego bloku indeksowego
				if (blocks.size() > BLOCK_DIRECT_INDEX) {
					file->directBlocks[BLOCK_DIRECT_INDEX] = std::make_shared<IndexBlock>();
					for (size_t i = 0; i < BLOCK_INDEX_NUMBER && i < blocks.size() - BLOCK_DIRECT_INDEX; i++) {
						std::dynamic_pointer_cast<IndexBlock>(file->directBlocks[BLOCK_DIRECT_INDEX])->value[i] = std::make_shared<Index>(blocks[i + BLOCK_DIRECT_INDEX]);
					}
				}

				//Dodanie pliku do obecnego katalogu
				currentDirectory->iNodes[name] = file;
				ArrayAdd(DISK.FileSystem.iNodeTable, currentDirectory->iNodes[name]);

				//Zapisanie danych pliku na dysku
				FileWrite(file, data);

				if (messages) { std::cout << "Stworzono plik o nazwie '" << name << "' w �cie�ce '" << GetCurrentPath() << "'.\n"; }
				return;
			}
			else { std::cout << "�cie�ka za d�uga!\n"; }
		}
		//Je�li plik si� nie mie�ci
		if (!CheckIfEnoughSpace(fileSize)) {
			std::cout << "Za ma�o miejsca!\n";
		}
		//Je�li plik jest wi�kszy ni� maksymalny dozwolony rozmiar
		if (data.size() > MAX_FILE_SIZE)
		{
			std::cout << "Rozmiar pliku wi�kszy ni� maksymalny dozwolony!\n";
		}
		//Je�li nazwa u�yta
		if (!CheckIfNameUnused(currentDirectory, name)) {
			std::cout << "Nazwa pliku '" << name << "' ju� zaj�ta!\n";
		}
	}
	else {
		std::cout << "Osi�gni�to limit element�w w �cie�ce '" << GetCurrentPath() << "'!\n";
	}
}

//!!!!!!!!!! NIEDOKO�CZONE !!!!!!!!!!
const std::string FileManager::FileOpen(const std::string &name) const
{
	return DISK.read<std::string>(0 * 8, 4 * 8 - 1);
}
//!!!!!!!!!! NIEDOKO�CZONE !!!!!!!!!!

const std::string FileManager::FileGetData(const std::shared_ptr<File>& file) const {
	//Dane
	std::string data;
	//Indeks do wczytywania danych z dysku
	size_t indexNumber = 0;
	std::shared_ptr<Index> index = file->directBlocks.value[indexNumber];

	//Dop�ki nie natrafimy na koniec pliku
	while (index != nullptr) {
		std::cout << "index value: " << (index != nullptr ? std::to_string(index->value) : "nullptr") << '\n';

		//Dodaje do danych fragment pliku pod wskazanym indeksem
		data += DISK.read<std::string>(index->value * BLOCK_SIZE, (index->value + 1)*BLOCK_SIZE - 1);
		//Przypisuje indeksowi kolejny indeks w tablicy FileSystem
		indexNumber++;
		if (indexNumber < BLOCK_DIRECT_INDEX) {
			index = file->directBlocks[indexNumber];
		}
		else if (file->directBlocks[BLOCK_DIRECT_INDEX] != nullptr) {
			index = (*std::dynamic_pointer_cast<IndexBlock>(file->directBlocks[BLOCK_DIRECT_INDEX]))[indexNumber - BLOCK_DIRECT_INDEX];
		}
		else { index = nullptr; }
	}
	return data;
}

void FileManager::FileDelete(const std::string &name) {
	//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
	const auto fileIterator = currentDirectory->iNodes.find(name);

	//Je�li znaleziono plik
	if (fileIterator != currentDirectory->iNodes.end()) {
		if (fileIterator->second->type == "FILE") {
			const std::shared_ptr<File> file = std::dynamic_pointer_cast<File>(fileIterator->second);
			FileDelete(file);
			//Usu� wpis o pliku z obecnego katalogu
			ArrayErase<iNode>(DISK.FileSystem.iNodeTable, fileIterator->second);
			currentDirectory->iNodes.erase(fileIterator);

			if (messages) { std::cout << "Usuni�to plik o nazwie '" << name << "' znajduj�cy si� w �cie�ce '" + GetCurrentPath() + "'.\n"; }
		}
		else { std::cout << "Nazwa " << name << " nie wskazuje na plik!\n"; }
	}
	else { std::cout << "Plik o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::FileTruncate(const std::string &name, const u_int &size) {
	//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
	const auto fileIterator = currentDirectory->iNodes.find(name);
	//Je�li znaleziono plik
	if (fileIterator != currentDirectory->iNodes.end()) {
		if (fileIterator->second->type == "FILE") {
			std::shared_ptr<File> file = std::dynamic_pointer_cast<File>(fileIterator->second);
			if (size <= (file->blocksOccupied - 1)*BLOCK_SIZE) {
				const u_int sizeToStart = u_int(ceil(double(size) / double(BLOCK_SIZE)))*BLOCK_SIZE;
				//Zmienna do analizowania, czy ju� mo�na usuwa� cz�� pliku
				u_int currentSize = 0;

				unsigned int indexNumber = 0;
				std::shared_ptr<Index> index = file->directBlocks[indexNumber];

				//Dop�ki indeks na co� wskazuje
				while (index != nullptr) {
					currentSize += BLOCK_SIZE;
					if (indexNumber < BLOCK_DIRECT_INDEX) {
						index = file->directBlocks[indexNumber];
					}
					else if (file->directBlocks[BLOCK_DIRECT_INDEX] != nullptr) {
						index = (*std::dynamic_pointer_cast<IndexBlock>(file->directBlocks[BLOCK_DIRECT_INDEX]))[indexNumber - BLOCK_DIRECT_INDEX];
					}
					else { index = nullptr; }
					//Spisz kolejny indeks
					indexNumber++;

					//Je�li obecny rozmiar przewy�sza rozmiar potrzebny do rozpocz�cia usuwania
					//zacznij usuwa� bloki
					if (currentSize > sizeToStart && index != nullptr) {
						//Zmniejszenie rozmiaru pliku
						file->blocksOccupied -= 1;
						//Po uci�ciu rozmiar i rozmiar rzeczywisty b�d� takie same
						file->sizeOnDisk = file->blocksOccupied*BLOCK_SIZE;
						//Oznacz obecny indeks jako wolny
						DISK.FileSystem.bitVector[index->value] = BLOCK_FREE;
						//Obecny indeks w tablicy FileSystem wskazuje na nic
						if (indexNumber < BLOCK_DIRECT_INDEX) {
							file->directBlocks[BLOCK_DIRECT_INDEX] = nullptr;
						}
						else if (file->directBlocks[BLOCK_DIRECT_INDEX] != nullptr) {
							(*std::dynamic_pointer_cast<IndexBlock>(file->directBlocks[BLOCK_DIRECT_INDEX]))[indexNumber - BLOCK_DIRECT_INDEX] = nullptr;
						}

					}
				}
				if (messages) { std::cout << "Zmniejszono plik o nazwie '" << name << "' do rozmiaru " << file->blocksOccupied*BLOCK_SIZE << " Bajt�w.\n"; }
			}
			else { std::cout << "Podano niepoprawny rozmiar!\n"; }
		}
		else { std::cout << "Nazwa " << name << " nie wskazuje na plik!\n"; }
	}
	else { std::cout << "Plik o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::DirectoryCreate(const std::string &name)
{
	if (fileNumber < TOTAL_FILE_NUMBER_LIMIT) {
		//Je�li w katalogu nie istnieje podkatalog o podanej nazwie
		if (currentDirectory->iNodes.find(name) == currentDirectory->iNodes.end()) {
			//Je�li �cie�ka nie przekracza maksymalnej d�ugo�ci
			if (name.size() + GetCurrentPathLength() < MAX_PATH_LENGTH) {
				fileNumber++;
				//Do iW�z��w w obecnym katalogu dodaj nowy podkatalog
				currentDirectory->iNodes[name] = std::make_shared<Directory>(name, currentDirectory);
				ArrayAdd(DISK.FileSystem.iNodeTable, currentDirectory->iNodes[name]);

				//Zapisanie daty stworzenia katalogu
				currentDirectory->creationTime = GetCurrentTimeAndDate();
				if (messages) {
					std::cout << "Stworzono katalog o nazwie '" << name << "' w �cie�ce '" << GetCurrentPath() << "'.\n";
				}
			}
			else { std::cout << "�cie�ka za d�uga!\n"; }
		}
		else { std::cout << "Nazwa katalogu '" << name << "' zaj�ta!\n"; }
	}
	else {
		std::cout << "Osi�gni�to limit element�w w �cie�ce '" << GetCurrentPath() << "'!\n";
	}
}

void FileManager::DirectoryDelete(const std::string &name) {
	//Iterator zwracany podczas przeszukiwania obecnego katalogu za katalogiem o podanej nazwie
	const auto directoryIterator = currentDirectory->iNodes.find(name);

	//Je�li znaleziono iNode
	if (directoryIterator != currentDirectory->iNodes.end()) {
		if (directoryIterator->second->type == "DIRECTORY") {
			//Wywo�aj funkcj� usuwania katalogu wraz z jego zawarto�ci�
			ArrayErase<iNode>(DISK.FileSystem.iNodeTable, directoryIterator->second);
			const std::shared_ptr<Directory> directory = std::dynamic_pointer_cast<Directory>(directoryIterator->second);
			DirectoryDeleteStructure(directory);
			//Usu� wpis o katalogu z obecnego katalogu
			currentDirectory->iNodes.erase(directoryIterator);
			if (messages) { std::cout << "Usuni�to katalog o nazwie '" << name << "' znajduj�cy si� w �cie�ce '" + GetCurrentPath() + "'.\n"; }
		}
		else { std::cout << "Nazwa " << name << " nie wskazuje na katalog!\n"; }
	}
	else { std::cout << "Katalog o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::DirectoryUp() {
	//Je�li istnieje katalog nadrz�dny
	if (currentDirectory->parentDirectory != nullptr) {
		//Przej�cie do katalogu nadrz�dnego
		currentDirectory = currentDirectory->parentDirectory;
		std::cout << "Obecna �cie�ka to '" << GetCurrentPath() << "'.\n";
	}
	else { std::cout << "Jeste� w katalogu g��wnym!\n"; }
}

void FileManager::DirectoryDown(const std::string &name) {
	const auto directoryIterator = currentDirectory->iNodes.find(name);

	//Je�li w obecnym katalogu znajduj� si� podkatalogi
	if (directoryIterator != currentDirectory->iNodes.end()) {
		if (directoryIterator->second->type == "DIRECTORY") {
			//Przej�cie do katalogu o wskazanej nazwie
			currentDirectory = std::dynamic_pointer_cast<Directory>(directoryIterator->second);
			std::cout << "Obecna �cie�ka to '" << GetCurrentPath() << "'.\n";
		}
		else { std::cout << "Nazwa " << name << " nie wskazuje na katalog!\n"; }
	}
	else { std::cout << "Katalog o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

//--------------------- Dodatkowe metody --------------------

void FileManager::FileRename(const std::string &name, const std::string &changeName) const
{
	//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
	const auto fileIterator = currentDirectory->iNodes.find(name);

	//Je�li znaleziono plik
	if (fileIterator != currentDirectory->iNodes.end()) {
		if (fileIterator->second->type == "FILE") {
			//Je�li plik si� zmie�ci i nazwa nie u�yta
			if (CheckIfNameUnused(currentDirectory, changeName)) {
				if (changeName.size() + GetCurrentPathLength() < MAX_PATH_LENGTH) {
					std::shared_ptr<File> file = std::dynamic_pointer_cast<File>(fileIterator->second);
					//Zapisywanie daty modyfikacji pliku
					file->modificationTime = GetCurrentTimeAndDate();

					//Lokowanie nowego klucza w tablicy hashowej i przypisanie do niego pliku
					currentDirectory->iNodes[changeName] = file;
					//Usuni�cie starego klucza
					currentDirectory->iNodes.erase(fileIterator);

					if (messages) { std::cout << "Zmieniono nazw� pliku '" << name << "' na '" << changeName << "'.\n"; }
				}
				else { std::cout << "�cie�ka za d�uga!\n"; }
			}
			else {
				std::cout << "Nazwa pliku '" << changeName << "' ju� zaj�ta!\n";
			}
		}
		else { std::cout << "Nazwa " << name << " nie wskazuje na plik!\n"; }
	}
	else { std::cout << "Plik o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::DirectoryRoot() {
	while (currentDirectory->parentDirectory != nullptr) {
		DirectoryUp();
	}
}

//------------------ Metody do wy�wietlania -----------------

void FileManager::Messages(const bool &onOff) {
	messages = onOff;
}

void FileManager::DisplayFileSystemParams() const {
	std::cout << "Disk capacity: " << DISK_CAPACITY << " Bytes\n";
	std::cout << "Block size: " << BLOCK_SIZE << " Bytes\n";
	std::cout << "Max file size: " << MAX_FILE_SIZE << " Bytes\n";
	std::cout << "Max indexes in block: " << BLOCK_INDEX_NUMBER << " Indexes\n";
	std::cout << "Max direct indexes in file: " << BLOCK_DIRECT_INDEX << " Indexes\n";
	std::cout << "Max file number: " << TOTAL_FILE_NUMBER_LIMIT << " Files\n";
	std::cout << "Max path length: " << MAX_PATH_LENGTH << " Characters\n";
	std::cout << "Number of files: " << fileNumber << " Files\n";
}

void FileManager::DisplayDirectoryInfo(const std::string &name) const
{
	const auto directoryIterator = currentDirectory->iNodes.find(name);
	if (directoryIterator != currentDirectory->iNodes.end()) {
		if (directoryIterator->second->type == "DIRECTORY") {
			const std::shared_ptr<Directory> directory = std::dynamic_pointer_cast<Directory>(directoryIterator->second);
			std::cout << "Name: " << directory->name << '\n';
			std::cout << "Size: " << CalculateDirectorySize(directory) << " Bytes\n";
			std::cout << "Size on disk: " << CalculateDirectorySizeOnDisk(directory) << " Bytes\n";
			std::cout << "Contains: " << CalculateDirectoryFileNumber(directory) << " Files, " << CalculateDirectoryFolderNumber(directory) << " Folders\n";
			std::cout << "Created: " << directory->creationTime << '\n';
		}
		else { std::cout << "Nazwa " << name << " nie wskazuje na katalog!\n"; }
	}
	else { std::cout << "Katalog o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::DisplayFileInfo(const std::string &name) const {
	const auto fileIterator = currentDirectory->iNodes.find(name);
	if (fileIterator != currentDirectory->iNodes.end()) {
		if (fileIterator->second->type == "FILE") {
			const std::shared_ptr<File> file = std::dynamic_pointer_cast<File>(fileIterator->second);
			std::cout << "Name: " << name << '\n';
			std::cout << "Size: " << file->blocksOccupied*BLOCK_SIZE << " Bytes\n";
			std::cout << "Size on disk: " << file->sizeOnDisk << " Bytes\n";
			std::cout << "Created: " << file->creationTime << '\n';
			std::cout << "Modified: " << file->modificationTime << '\n';
			std::cout << "Saved data: " << FileGetData(file) << '\n';
		}
		else { std::cout << "Nazwa " << name << " nie wskazuje na plik!\n"; }
	}
	else { std::cout << "Plik o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::DisplayDirectoryStructure() const {
	DisplayDirectory(DISK.FileSystem.rootDirectory, 1);
}
void FileManager::DisplayDirectory(const std::shared_ptr<Directory>& directory, u_int level) {
	std::cout << std::string(level, ' ') << directory->name << "\\\n";
	for (auto i = directory->iNodes.begin(); i != directory->iNodes.end(); ++i) {
		if (i->second->type == "FILE") {
			std::cout << std::string(level + 1, ' ') << "- " << i->first << '\n';
		}
	}
	level++;
	for (auto i = directory->iNodes.begin(); i != directory->iNodes.end(); ++i) {
		if (i->second->type == "DIRECTORY") {
			DisplayDirectory(std::dynamic_pointer_cast<Directory>(i->second), level);
		}
	}
}

void FileManager::DisplayDiskContentBinary() {
	u_int index = 0;
	for (const char &c : DISK.space) {
		//bitset - tablica bitowa
		std::cout << std::bitset<8>(c) << (index % BLOCK_SIZE == BLOCK_SIZE - 1 ? " , " : "") << (index % 16 == 15 ? " \n" : " ");
		index++;
	}
	std::cout << '\n';
}

void FileManager::DisplayDiskContentChar() {
	u_int index = 0;
	for (const char &c : DISK.space) {
		if (c == ' ') { std::cout << ' '; }
		else if (c >= 0 && c <= 32) std::cout << ".";
		else std::cout << c;
		std::cout << (index % BLOCK_SIZE == BLOCK_SIZE - 1 ? " , " : "") << (index % 32 == 31 ? " \n" : " ");
		index++;
	}
	std::cout << '\n';
}

//void FileManager::DisplayFileAllocationTable() {
//	u_int index = 0;
//	for (u_int i = 0; i < DISK.FileSystem.FileAllocationTable.size(); i++) {
//		if (i % 8 == 0) { std::cout << std::setfill('0') << std::setw(2) << (index / 8) + 1 << ". "; }
//		std::cout << std::setfill('0') << std::setw(3) << (DISK.FileSystem.FileAllocationTable[i] != u_int(-1) ? std::to_string(DISK.FileSystem.FileAllocationTable[i]) : "NUL")
//			<< (index % 8 == 7 ? "\n" : " ");
//		index++;
//	}
//	std::cout << '\n';
//}

void FileManager::DisplayBitVector() {
	u_int index = 0;
	for (u_int i = 0; i < DISK.FileSystem.bitVector.size(); i++) {
		if (i % 8 == 0) { std::cout << std::setfill('0') << std::setw(2) << (index / 8) + 1 << ". "; }
		std::cout << DISK.FileSystem.bitVector[i] << (index % 8 == 7 ? "\n" : " ");
		index++;
	}
	std::cout << '\n';
}

void FileManager::DisplayFileFragments(const std::vector<std::string> &fileFragments)
{
	for (const auto& fileFragment : fileFragments)
	{
		std::cout << fileFragment << std::string(BLOCK_SIZE - 1 - fileFragment.size(), ' ') << '\n';
	}
}

//-------------------- Metody Pomocnicze --------------------
template <typename T, size_t size>
void FileManager::ArrayAdd(std::array<std::shared_ptr<T>, size>& array, std::shared_ptr<T> input) {
	for (std::shared_ptr<T> &ptr : array)
	{
		if (ptr == nullptr) { ptr = input; break; }
	}
}

template <typename T, size_t size>
void FileManager::ArrayErase(std::array<std::shared_ptr<T>, size>& array, std::shared_ptr<T> input) {
	for (std::shared_ptr<T> &ptr : array)
	{
		if (ptr == input) { ptr = nullptr; break; }
	}
}

void FileManager::DirectoryDeleteStructure(const std::shared_ptr<Directory>& directory) {
	fileNumber--;
	//Usuwa wszystkie pliki z katalogu
	for (auto i = directory->iNodes.begin(); i != directory->iNodes.end(); ++i) {
		if (i->second->type == "FILE") {
			const std::shared_ptr<File> file = std::dynamic_pointer_cast<File>(i->second);
			FileDelete(file);
			if (messages) { std::cout << "Usuni�to plik o nazwie '" << i->first << "' znajduj�cy si� w �cie�ce '" + GetPath(directory) + "'.\n"; }
		}
	}

	//Usuwa wszystkie katalogi w katalogu
	for (auto i = directory->iNodes.begin(); i != directory->iNodes.end(); ++i) {
		if (i->second->type == "DIRECTORY") {
			//Wywo�anie funkcji na podrz�dnym katalogu
			const std::shared_ptr<Directory> dir = std::dynamic_pointer_cast<Directory>(i->second);
			DirectoryDeleteStructure(dir);
			if (messages) { std::cout << "Usuni�to katalog o nazwie '" << i->first << "' znajduj�cy si� w �cie�ce '" + GetPath(dir) + "'.\n"; }
		}
	}
	//Czy�ci list� iW�z��w w katalogu
	directory->iNodes.clear();
	ArrayErase<iNode>(DISK.FileSystem.iNodeTable, directory);
}

void FileManager::FileDelete(const std::shared_ptr<File>& file) {
	if (file != nullptr) {
		fileNumber--;
		//Obecny indeks
		u_int indexNumber = 0;
		std::shared_ptr<Index> index = file->directBlocks[indexNumber];
		//Dop�ki indeks na co� wskazuje
		while (index != nullptr) {
			//Oznacz obecny indeks jako wolny
			DISK.FileSystem.bitVector[index->value] = BLOCK_FREE;
			//Obecny indeks w tablicy FileSystem wskazuje na nic
			file->directBlocks[indexNumber] = nullptr;
			//Przypisz do obecnego indeksu kolejny indeks
			indexNumber++;
			if (indexNumber < BLOCK_DIRECT_INDEX) {
				index = file->directBlocks[indexNumber];
			}
			else if (file->directBlocks[BLOCK_DIRECT_INDEX] != nullptr) {
				index = (*std::dynamic_pointer_cast<IndexBlock>(file->directBlocks[BLOCK_DIRECT_INDEX]))[indexNumber - BLOCK_DIRECT_INDEX];
			}
			else { index = nullptr; }
		}
	}
}

const size_t FileManager::CalculateDirectorySize(const std::shared_ptr<Directory>& directory)
{
	if (directory != nullptr) {
		//Rozmiar katalogu
		size_t size = 0;

		//Dodaje rozmiar plik�w w katalogu do rozmiaru katalogu
		for (const auto &element : directory->iNodes) {
			if (element.second->type == "FILE") {
				size += std::dynamic_pointer_cast<File>(element.second)->blocksOccupied*BLOCK_SIZE;
			}
		}
		//Przegl�da katalogi i wywo�uje na nich obecn� funkcj� i dodaje zwr�con� warto�� do rozmiaru
		for (const auto &element : directory->iNodes) {
			if (element.second->type == "DIRECTORY") {
				size += CalculateDirectorySize(std::dynamic_pointer_cast<Directory>(element.second));
			}
		}
		return size;
	}
	else { return 0; }
}

const size_t FileManager::CalculateDirectorySizeOnDisk(const std::shared_ptr<Directory>& directory)
{
	if (directory != nullptr) {
		//Rzeczywisty rozmiar katalogu
		size_t sizeOnDisk = 9;

		//Dodaje rzeczywisty rozmiar plik�w w katalogu do rozmiaru katalogu
		for (const auto &element : directory->iNodes) {
			if (element.second->type == "FILE") {
				sizeOnDisk += std::dynamic_pointer_cast<File>(element.second)->sizeOnDisk;
			}
		}
		//Przegl�da katalogi i wywo�uje na nich obecn� funkcj� i dodaje zwr�con� warto�� do rozmiaru
		for (const auto &element : directory->iNodes) {
			if (element.second->type == "DIRECTORY") {
				sizeOnDisk += CalculateDirectorySize(std::dynamic_pointer_cast<Directory>(element.second));
			}
		}

		return sizeOnDisk;
	}
	else { return 0; }
}

const u_int FileManager::CalculateDirectoryFolderNumber(const std::shared_ptr<Directory>& directory)
{
	if (directory != nullptr) {
		//Ilo�� folder�w w danym katalogu
		u_int folderNumber = 0;

		/**
		 Dodaje ilo�� folder�w w tym katalogu do zwracanej zmiennej
		 Przegl�da katalogi i wywo�uje na nich obecn� funkcj� i dodaje zwr�con� warto�� do ilo�ci
		 */
		for (const auto &element : directory->iNodes) {
			if (element.second->type == "DIRECTORY") {
				folderNumber += CalculateDirectoryFolderNumber(std::dynamic_pointer_cast<Directory>(element.second));
				folderNumber += 1;
			}
		}
		return folderNumber;
	}
	else { return 0; }
}

const u_int FileManager::CalculateDirectoryFileNumber(const std::shared_ptr<Directory>& directory)
{
	if (directory != nullptr) {
		//Ilo�� folder�w w danym katalogu
		u_int fileNumber = 0;

		/**
		 Dodaje ilo�� plik�w w tym katalogu do zwracanej zmiennej
		 Przegl�da katalogi i wywo�uje na nich obecn� funkcj� i dodaje zwr�con� warto�� do ilo�ci
		 */
		for (const auto &element : directory->iNodes) {
			if (element.second->type == "FILE") {
				fileNumber += CalculateDirectoryFileNumber(std::dynamic_pointer_cast<Directory>(element.second));
				fileNumber += 1;
			}
		}
		return fileNumber;
	}
	else { return 0; }
}

const std::string FileManager::GetCurrentPath() const {
	//�cie�ka
	std::string path;
	//Tymczasowa zmienna przechowuj�ca wska�nik na katalog
	std::shared_ptr<Directory> tempDir = currentDirectory;
	//Dop�ki nie doszli�my do pustego katalogu
	while (tempDir != nullptr) {
		//Dodaj do �cie�ki od przodu nazw� obecnego katalogu
		path.insert(0, "/" + tempDir->name);
		//Przypisanie tymczasowej zmiennej katalog wy�szy w hierarchii
		tempDir = tempDir->parentDirectory;
	}
	return path;
}

const std::string FileManager::GetPath(std::shared_ptr<Directory> directory) {
	std::string path;
	std::shared_ptr<Directory> tempDir = std::move(directory);
	//Dop�ki nie doszli�my do pustego katalogu
	while (tempDir != nullptr) {
		//Dodaj do �cie�ki od przodu nazw� obecnego katalogu
		path.insert(0, "/" + tempDir->name);
		//Przypisanie tymczasowej zmiennej katalog wy�szy w hierarchii
		tempDir = tempDir->parentDirectory;
	}
	return path;
}

const tm FileManager::GetCurrentTimeAndDate()
{
	time_t tt;
	time(&tt);
	tm timeAndDate = *localtime(&tt);
	timeAndDate.tm_year += 1900;
	timeAndDate.tm_mon += 1;
	return timeAndDate;
}

const size_t FileManager::GetCurrentPathLength() const
{
	//�cie�ka
	size_t length = 0;
	//Tymczasowa zmienna przechowuj�ca wska�nik na katalog
	std::shared_ptr<Directory> tempDir = currentDirectory;
	//Dop�ki nie doszli�my do pustego katalogu
	while (tempDir != nullptr) {
		//Dodaj do �cie�ki od przodu nazw� obecnego katalogu
		length += tempDir->name.size();
		//Przypisanie tymczasowej zmiennej katalog wy�szy w hierarchii
		tempDir = tempDir->parentDirectory;
	}
	return length;
}

const bool FileManager::CheckIfNameUnused(const std::shared_ptr<Directory>& directory, const std::string &name)
{
	//Przeszukuje podany katalog za plikiem o tej samej nazwie
	for (auto i = directory->iNodes.begin(); i != directory->iNodes.end(); ++i) {
		//Je�li nazwa ta sama
		if (i->first == name) { return false; }
	}
	return true;
}

const bool FileManager::CheckIfEnoughSpace(const u_int &dataSize) const
{
	//Je�li dane si� mieszcz�
	if (dataSize <= DISK.FileSystem.freeSpace) { return true; }
	//Je�li dane si� nie mieszcz�
	return false;
}

void FileManager::ChangeBitVectorValue(const u_int &block, const bool &value) {
	//Je�li warto�� zaj�ty to wolne miejsce - BLOCK_SIZE
	if (value == 1) { DISK.FileSystem.freeSpace -= BLOCK_SIZE; }
	//Je�li warto�� wolny to wolne miejsce + BLOCK_SIZE
	else if (value == 0) { DISK.FileSystem.freeSpace += BLOCK_SIZE; }
	//Przypisanie blokowi podanej warto�ci
	DISK.FileSystem.bitVector[block] = value;
}

void FileManager::FileWrite(const std::shared_ptr<File> &file, const std::string &data) {
	//Uzyskuje dane podzielone na fragmenty
	const std::vector<std::string>fileFragments = DataToDataFragments(data);
	//Index pod kt�rym maj� zapisywane by� dane
	u_int indexNumber = 0;
	std::shared_ptr<Index> index = file->directBlocks[indexNumber];



	//Zapisuje wszystkie dane na dysku
	for (const auto& fileFragment : fileFragments)
	{
		//Zapisuje fragment na dysku
		DISK.write(index->value * BLOCK_SIZE, index->value * BLOCK_SIZE + fileFragment.size() - 1, fileFragment);
		//Zmienia warto�� bloku w wektorze bitowym na zaj�ty
		ChangeBitVectorValue(index->value, BLOCK_OCCUPIED);
		//Przypisuje do indeksu numer kolejnego bloku
		indexNumber++;
		if (indexNumber < BLOCK_DIRECT_INDEX) {
			index = file->directBlocks[indexNumber];
		}
		else if (file->directBlocks[BLOCK_DIRECT_INDEX] != nullptr) {
			index = (*std::dynamic_pointer_cast<IndexBlock>(file->directBlocks[BLOCK_DIRECT_INDEX]))[indexNumber - BLOCK_DIRECT_INDEX];
		}
	}
}

const std::vector<std::string> FileManager::DataToDataFragments(const std::string &data) const
{
	//Tablica fragment�w podanych danych
	std::vector<std::string>fileFragments;

	//Przetworzenie ca�ych danych
	for (u_int i = 0; i < CalculateNeededBlocks(data); i++) {
		//Oblicza pocz�tek kolejnej cz�ci fragmentu danych.
		const u_int substrBegin = i * BLOCK_SIZE;
		//Dodaje do tablicy fragment�w kolejny fragment o d�ugo�ci BLOCK_SIZE
		fileFragments.push_back(data.substr(substrBegin, BLOCK_SIZE));
	}
	return fileFragments;
}

const u_int FileManager::CalculateNeededBlocks(const std::string &data) const
{
	/*
	Przybli�enie w g�r� rozmiaru pliku przez rozmiar bloku.
	Jest tak, poniewa�, je�li zape�nia chocia� o jeden bajt
	wi�cej przy zaj�tym bloku, to trzeba zaalokowa� wtedy kolejny blok.
	*/
	return int(ceil(double(data.size()) / double(BLOCK_SIZE)));
}

const std::vector<u_int> FileManager::FindUnallocatedBlocksFragmented(u_int blockNumber) {
	//Lista wolnych blok�w
	std::vector<u_int> blockList;

	//Szuka wolnych blok�w
	for (u_int i = 0; i < DISK.FileSystem.bitVector.size(); i++) {
		//Je�li blok wolny
		if (DISK.FileSystem.bitVector[i] == BLOCK_FREE) {
			//Dodaje indeks bloku
			blockList.push_back(i);
			//Potrzeba teraz jeden blok mniej
			blockNumber--;
			//Je�li potrzeba 0 blok�w, przerwij
			if (blockNumber == 0) { break; }
		}
	}
	blockList.push_back(-1);
	return blockList;
}

const std::vector<u_int> FileManager::FindUnallocatedBlocksBestFit(const u_int &blockNumber) {
	//Lista indeks�w blok�w (dopasowanie)
	std::vector<u_int> blockList;
	//Najlepsze dopasowanie
	std::vector<u_int> bestBlockList(DISK.FileSystem.bitVector.size() + 1);

	//Szukanie wolnych blok�w spe�niaj�cych minimum miejsca
	for (u_int i = 0; i < DISK.FileSystem.bitVector.size(); i++) {
		//Je�li blok wolny
		if (DISK.FileSystem.bitVector[i] == BLOCK_FREE) {
			//Dodaj indeks bloku do listy blok�w
			blockList.push_back(i);
		}
		//Je�li blok zaj�ty
		else {
			//Je�li uzyskana lista blok�w jest wi�ksza od ilo�ci blok�w jak� chcemy uzyska�
			//to dodaj uzyskane dopasowanie do listy dopasowa�;
			if (blockList.size() >= blockNumber) {
				//Je�li znalezione dopasowanie mniejsze ni� najlepsze dopasowanie
				if (blockList.size() < bestBlockList.size()) {
					//Przypisanie nowego najlepszego dopasowania
					bestBlockList = blockList;
					if (bestBlockList.size() == blockNumber) { break; }
				}
			}

			//Czy�ci list� blok�w, aby mo�na przygotowa� kolejne dopasowanie
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
		//Je�li blok wolny
		if (blockList.size() < bestBlockList.size()) {
			//Dodaj indeks bloku do listy blok�w
			bestBlockList = blockList;
		}
	}

	//Je�li znalezione najlepsze dopasowanie
	if (bestBlockList.size() < DISK.FileSystem.bitVector.size() + 1) {
		//Odetnij nadmiarowe indeksy z dopasowania (je�li wi�ksze ni� potrzeba)
		bestBlockList.resize(blockNumber);
	}
	//Inaczej zmniejsz dopasowanie do 0, �eby po zwr�ceniu wybrano inn� metod�
	else { bestBlockList.resize(0); }

	return bestBlockList;
}

const std::vector<u_int> FileManager::FindUnallocatedBlocks(const u_int &blockNumber) {
	//Szuka blok�w funkcj� z metod� best-fit
	std::vector<u_int> blockList = FindUnallocatedBlocksBestFit(blockNumber);

	//Je�li funkcja z metod� best-fit nie znajdzie dopasowa�
	if (blockList.empty()) {
		//Szuka niezaalokowanych blok�w, wybieraj�c pierwsze wolne
		blockList = FindUnallocatedBlocksFragmented(blockNumber);
	}

	return blockList;
}

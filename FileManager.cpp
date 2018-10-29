/**
	SexyOS
	FileManager.cpp
	Przeznaczenie: Zawiera definicje metod i konstruktor�w dla klas z FileManager.h

	@author Tomasz Kilja�czyk
	@version 29/10/18
*/

#include "FileManager.h"
#include <algorithm>
#include <iomanip>
#include <iostream>

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

FileManager::Disk::FileSystem::FileSystem() {
	std::fill(FileAllocationTable.begin(), FileAllocationTable.end(), -1);
}

void FileManager::Disk::write(const unsigned int &begin, const unsigned int &end, const std::string &data) {
	//Indeks kt�ry b�dzie s�u�y� do wskazywania na kom�rki pami�ci
	unsigned int index = begin;
	//Iterowanie po danych typu string i zapisywanie znak�w na dysku
	for (unsigned int i = 0; i < data.size() && i <= end - begin; i++) {
		space[index] = data[i];
		index++;
	}
	//Zapisywanie NULL, je�li dane nie wype�ni�y ostatniego bloku
	for (; index <= end; index++) {
		space[index] = NULL;
	}
}

void FileManager::Disk::write(const unsigned int &index, const unsigned int &data) {
	//Zapisz liczb� pod danym indeksem
	space[index] = data;
}

template<typename T>
const T FileManager::Disk::read(const unsigned int &begin, const unsigned int &end) {
	//Dane
	T data;

	//Je�li typ danych to string
	if (typeid(T) == typeid(std::string)) {
		//Odczytaj przestrze� dyskow� od indeksu begin do indeksu end
		for (unsigned int index = begin; index <= end; index++) {
			//Dodaj znak zapisany na dysku do danych
			data += space[index];
		}
	}

	return data;
}

//----------------------- FileManager  ----------------------

FileManager::FileManager() {
	//Przypisanie katalogu g��wnego do obecnego katalogu 
	currentDirectory = &DISK.FileSystem.rootDirectory;
}

//-------------------- Podstawowe Metody --------------------

void FileManager::FileCreate(const std::string &name, const std::string &data) {
	//Rozmiar pliku obliczony na podstawie podanych danych
	const unsigned int fileSize = CalculateNeededBlocks(data)*BLOCK_SIZE;

	if (currentDirectory->files.size() + currentDirectory->subDirectories.size() < MAX_DIRECTORY_ELEMENTS) {
		//Je�li plik si� zmie�ci i nazwa nie u�yta
		if (CheckIfEnoughSpace(fileSize) && CheckIfNameUnused(*currentDirectory, name)) {
			//Je�li �cie�ka nie przekracza maksymalnej d�ugo�ci
			if (name.size() + GetCurrentPathLength() < MAX_PATH_LENGTH) {
				//Stw�rz plik o podanej nazwie
				File file = File(name);
				//Zapisz w pliku jego rozmiar
				file.size = fileSize;
				//Zapisz w plik jego rzeczywisty rozmiar
				file.sizeOnDisk = data.size();

				//Zapisywanie daty stworzenia pliku
				file.creationTime = GetCurrentTimeAndDate();
				file.modificationTime = file.creationTime;

				//Lista indeks�w blok�w, kt�re zostan� zaalokowane na potrzeby pliku
				const std::vector<unsigned int> blocks = FindUnallocatedBlocks(file.size / BLOCK_SIZE);

				//Wpisanie blok�w do tablicy FileSystem
				for (unsigned int i = 0; i < blocks.size() - 1; i++) {
					DISK.FileSystem.FileAllocationTable[blocks[i]] = blocks[i + 1];
				}

				//Dodanie do pliku indeksu pierwszego bloku na kt�rym jest zapisany
				file.FileSystemIndex = blocks[0];

				//Dodanie pliku do obecnego katalogu
				currentDirectory->files[file.name] = file;

				//Zapisanie danych pliku na dysku
				WriteFile(file, data);

				if (messages) { std::cout << "Stworzono plik o nazwie '" << file.name << "' w �cie�ce '" << GetCurrentPath() << "'.\n"; }
				return;
			}
			else { std::cout << "�cie�ka za d�uga!\n"; }
		}
		//Je�li plik si� nie mie�ci
		if (!CheckIfEnoughSpace(fileSize)) {
			std::cout << "Za ma�o miejsca!\n";
		}
		//Je�li nazwa u�yta
		if (!CheckIfNameUnused(*currentDirectory, name)) {
			std::cout << "Nazwa pliku '" << name << "' ju� zaj�ta!\n";
		}
	}
	else {
		std::cout << "Osi�gni�to limit element�w w �cie�ce '" << GetCurrentPath() << "'!\n";
	}
}

//!!!!!!!!!! NIEDOKO�CZONE !!!!!!!!!!
const std::string FileManager::FileOpen(const std::string &name) {
	return DISK.read<std::string>(0 * 8, 4 * 8 - 1);
}
//!!!!!!!!!! NIEDOKO�CZONE !!!!!!!!!!

const std::string FileManager::FileGetData(const File &file) {
	//Dane
	std::string data;
	//Indeks do wczytywania danych z dysku
	unsigned int index = file.FileSystemIndex;
	//Dop�ki nie natrafimy na koniec pliku
	while (index != unsigned int(-1)) {
		//Dodaje do danych fragment pliku pod wskazanym indeksem
		data += DISK.read<std::string>(index*BLOCK_SIZE, (index + 1)*BLOCK_SIZE - 1);
		//Przypisuje indeksowi kolejny indeks w tablicy FileSystem
		index = DISK.FileSystem.FileAllocationTable[index];
	}
	return data;
}

void FileManager::FileDelete(const std::string &name) {
	//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
	auto fileIterator = currentDirectory->files.find(name);

	//Je�li znaleziono plik
	if (fileIterator != currentDirectory->files.end()) {
		FileDelete(fileIterator->second);
		//Usu� wpis o pliku z obecnego katalogu
		currentDirectory->files.erase(fileIterator);

		if (messages) { std::cout << "Usuni�to plik o nazwie '" << name << "' znajduj�cy si� w �cie�ce '" + GetCurrentPath() + "'.\n"; }
	}
	else { std::cout << "Plik o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::FileTruncate(const std::string &name, const unsigned int &size) {
	//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
	auto fileIterator = currentDirectory->files.find(name);
	//Je�li znaleziono plik
	if (fileIterator != currentDirectory->files.end()) {
		if (size <= fileIterator->second.size - BLOCK_SIZE) {
			const unsigned int sizeToStart = unsigned int(ceil(double(size) / double(BLOCK_SIZE)))*BLOCK_SIZE;
			//Zmienna do analizowania, czy ju� mo�na usuwa� cz�� pliku
			unsigned int currentSize = 0;
			//Obecny indeks
			unsigned index = fileIterator->second.FileSystemIndex;
			//Dop�ki indeks na co� wskazuje
			while (index != unsigned int(-1)) {
				//Zwi�ksz obecny rozmiar o rozmiar jednostki alokacji
				currentSize += BLOCK_SIZE;
				//Spisz kolejny indeks
				const unsigned int tempIndex = DISK.FileSystem.FileAllocationTable[index];

				//Je�li obecny rozmiar przewy�sza rozmiar potrzebny do rozpocz�cia usuwania
				//zacznij usuwa� bloki
				if (currentSize > sizeToStart) {
					//Zmniejszenie rozmiaru pliku
					fileIterator->second.size -= BLOCK_SIZE;
					//Po uci�ciu rozmiar i rozmiar rzeczywisty b�d� takie same
					fileIterator->second.sizeOnDisk = fileIterator->second.size;
					//Oznacz obecny indeks jako wolny
					DISK.FileSystem.bitVector[index] = BLOCK_FREE;
					//Obecny indeks w tablicy FileSystem wskazuje na nic
					DISK.FileSystem.FileAllocationTable[index] = -1;
				}
				//Przypisz do obecnego indeksu kolejny indeks
				index = tempIndex;
			}
			if (messages) { std::cout << "Zmniejszono plik o nazwie '" << name << "' do rozmiaru " << fileIterator->second.size << " Bajt�w.\n"; }
		}
		else { std::cout << "Podano niepoprawny rozmiar!\n"; }
	}
	else { std::cout << "Plik o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::DirectoryCreate(const std::string &name) const
{
	if (currentDirectory->files.size() + currentDirectory->subDirectories.size() < MAX_DIRECTORY_ELEMENTS) {
		//Je�li w katalogu nie istnieje podkatalog o podanej nazwie
		if (currentDirectory->subDirectories.find(name) == currentDirectory->subDirectories.end()) {
			//Je�li �cie�ka nie przekracza maksymalnej d�ugo�ci
			if (name.size() + GetCurrentPathLength() < MAX_PATH_LENGTH) {
				//Do podkatalog�w obecnego katalogu dodaj nowy katalog o podanej nazwie
				currentDirectory->subDirectories[name] = Directory(name, &(*currentDirectory));
				//Zapisanie daty stworzenia katalogu
				currentDirectory->creationTime = GetCurrentTimeAndDate();
				if (messages) {
					std::cout << "Stworzono katalog o nazwie '" << currentDirectory->subDirectories[name].name
						<< "' w �cie�ce '" << GetCurrentPath() << "'.\n";
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
	auto directoryIterator = currentDirectory->subDirectories.find(name);

	//Je�li znaleziono plik
	if (directoryIterator != currentDirectory->subDirectories.end()) {
		//Wywo�aj funkcj� usuwania katalogu wraz z jego zawarto�ci�
		DirectoryDeleteStructure(directoryIterator->second);
		//Usu� wpis o katalogu z obecnego katalogu
		currentDirectory->subDirectories.erase(directoryIterator);
		if (messages) { std::cout << "Usuni�to katalog o nazwie '" << name << "' znajduj�cy si� w �cie�ce '" + GetCurrentPath() + "'.\n"; }
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
	//Je�li w obecnym katalogu znajduj� si� podkatalogi
	if (currentDirectory->subDirectories.find(name) != currentDirectory->subDirectories.end()) {
		//Przej�cie do katalogu o wskazanej nazwie
		currentDirectory = &(currentDirectory->subDirectories.find(name)->second);
		std::cout << "Obecna �cie�ka to '" << GetCurrentPath() << "'.\n";
	}
	else { std::cout << "Brak katalogu o podanej nazwie!\n"; }
}

//--------------------- Dodatkowe metody --------------------

void FileManager::FileRename(const std::string &name, const std::string &changeName) const
{
	//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
	auto file = currentDirectory->files.find(name);

	//Je�li znaleziono plik
	if (file != currentDirectory->files.end()) {
		//Je�li plik si� zmie�ci i nazwa nie u�yta
		if (CheckIfNameUnused(*currentDirectory, changeName)) {
			if (changeName.size() + GetCurrentPathLength() < MAX_PATH_LENGTH) {

				//Zapisywanie daty modyfikacji pliku
				file->second.modificationTime = GetCurrentTimeAndDate();

				//Zmiana nazwy pliku
				file->second.name = changeName;

				//Lokowanie nowego klucza w tablicy hashowej i przypisanie do niego pliku
				currentDirectory->files[changeName] = file->second;
				//Usuni�cie starego klucza
				currentDirectory->files.erase(file);

				if (messages) { std::cout << "Zmieniono nazw� pliku '" << name << "' na '" << currentDirectory->files[changeName].name << "'.\n"; }
				return;
			}
			else { std::cout << "�cie�ka za d�uga!\n"; }
		}
		else {
			std::cout << "Nazwa pliku '" << changeName << "' ju� zaj�ta!\n";
		}
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

void FileManager::DisplayDirectoryInfo(const std::string &name) const
{
	const auto directoryIterator = currentDirectory->subDirectories.find(name);
	if (directoryIterator != currentDirectory->subDirectories.end()) {
		const Directory directory = directoryIterator->second;
		std::cout << "Name: " << directory.name << '\n';
		std::cout << "Size: " << CalculateDirectorySize(directory) << " Bytes\n";
		std::cout << "Size on disk: " << CalculateDirectorySize(directory) << " Bytes\n";
		std::cout << "Contains: " << CalculateDirectoryFileCount(directory) << " Files, " << CalculateDirectoryFolderCount(directory) << " Folders\n";
		std::cout << "Created: " << directory.creationTime << '\n';
	}
	else { std::cout << "Katalog o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::DisplayFileInfo(const std::string &name) {
	const auto fileIterator = currentDirectory->files.find(name);
	if (fileIterator != currentDirectory->files.end()) {
		const File file = fileIterator->second;
		std::cout << "Name: " << file.name << '\n';
		std::cout << "Size: " << file.size << " Bytes\n";
		std::cout << "Size on disk: " << file.sizeOnDisk << " Bytes\n";
		std::cout << "Created: " << file.creationTime << '\n';
		std::cout << "Modified: " << file.modificationTime << '\n';
		std::cout << "FileSystem index: " << file.FileSystemIndex << '\n';
		std::cout << "Saved data: " << FileGetData(file) << '\n';
	}
	else { std::cout << "Plik o nazwie '" << name << "' nie znaleziony w �cie�ce '" + GetCurrentPath() + "'!\n"; }
}

void FileManager::DisplayDirectoryStructure() const
{
	DisplayDirectory(DISK.FileSystem.rootDirectory, 1);
}
void FileManager::DisplayDirectory(const Directory &directory, unsigned int level)
{
	std::cout << std::string(level, ' ') << directory.name << "\\\n";
	for (auto i = directory.files.begin(); i != directory.files.end(); ++i) {
		std::cout << std::string(level + 1, ' ') << "- " << i->first << '\n';
	}
	level++;
	for (auto i = directory.subDirectories.begin(); i != directory.subDirectories.end(); ++i) {
		DisplayDirectory(i->second, level);
	}
}

void FileManager::DisplayDiskContentBinary() {
	unsigned int index = 0;
	for (const char &c : DISK.space) {
		//bitset - tablica bitowa
		std::cout << std::bitset<8>(c) << (index % BLOCK_SIZE == BLOCK_SIZE - 1 ? " , " : "") << (index % 16 == 15 ? " \n" : " ");
		index++;
	}
	std::cout << '\n';
}

void FileManager::DisplayDiskContentChar() {
	unsigned int index = 0;
	for (const char &c : DISK.space) {
		if (c == ' ') { std::cout << ' '; }
		else if (c >= 0 && c <= 32) std::cout << ".";
		else std::cout << c;
		std::cout << (index % BLOCK_SIZE == BLOCK_SIZE - 1 ? " , " : "") << (index % 32 == 31 ? " \n" : " ");
		index++;
	}
	std::cout << '\n';
}

void FileManager::DisplayFileAllocationTable() {
	unsigned int index = 0;
	for (unsigned int i = 0; i < DISK.FileSystem.FileAllocationTable.size(); i++) {
		if (i % 8 == 0) { std::cout << std::setfill('0') << std::setw(2) << (index / 8) + 1 << ". "; }
		std::cout << std::setfill('0') << std::setw(3) << (DISK.FileSystem.FileAllocationTable[i] != unsigned int(-1) ? std::to_string(DISK.FileSystem.FileAllocationTable[i]) : "NUL")
			<< (index % 8 == 7 ? "\n" : " ");
		index++;
	}
	std::cout << '\n';
}

void FileManager::DisplayBitVector() {
	unsigned int index = 0;
	for (unsigned int i = 0; i < DISK.FileSystem.bitVector.size(); i++) {
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

void FileManager::DirectoryDeleteStructure(Directory &directory) {
	//Usuwa wszystkie pliki z katalogu
	for (auto i = directory.files.begin(); i != directory.files.end(); ++i) {
		FileDelete(i->second);
		if (messages) { std::cout << "Usuni�to plik o nazwie '" << i->second.name << "' znajduj�cy si� w �cie�ce '" + GetPath(directory) + "'.\n"; }
	}
	//Czy�ci list� plik�w w katalogu
	directory.files.clear();
	//Usuwa wszystkie katalogi w katalogu
	for (auto i = directory.subDirectories.begin(); i != directory.subDirectories.end(); ++i) {
		//Wywo�anie funkcji na podrz�dnym katalogu
		DirectoryDeleteStructure(i->second);
		if (messages) { std::cout << "Usuni�to katalog o nazwie '" << i->second.name << "' znajduj�cy si� w �cie�ce '" + GetPath(directory) + "'.\n"; }
	}
	//Czy�ci list� katalog�w w katalogu
	directory.subDirectories.clear();
}

void FileManager::FileDelete(File &file) {
	//Obecny indeks
	unsigned index = file.FileSystemIndex;
	//Dop�ki indeks na co� wskazuje
	while (index != unsigned int(-1)) {
		//Spisz kolejny indeks
		const unsigned int tempIndex = DISK.FileSystem.FileAllocationTable[index];
		//Oznacz obecny indeks jako wolny
		DISK.FileSystem.bitVector[index] = BLOCK_FREE;
		//Obecny indeks w tablicy FileSystem wskazuje na nic
		DISK.FileSystem.FileAllocationTable[index] = -1;
		//Przypisz do obecnego indeksu kolejny indeks
		index = tempIndex;
	}
}

const size_t FileManager::CalculateDirectorySize(const Directory &directory)
{
	//Rozmiar katalogu
	size_t size = 0;

	//Dodaje rozmiar plik�w w katalogu do rozmiaru katalogu
	for (const auto &file : directory.files) {
		size += file.second.size;
	}
	//Przegl�da katalogi i wywo�uje na nich obecn� funkcj� i dodaje zwr�con� warto�� do rozmiaru
	for (const auto &dir : directory.subDirectories) {
		size += CalculateDirectorySize(dir.second);
	}

	return size;
}

const size_t FileManager::CalculateDirectorySizeOnDisk(const Directory &directory)
{
	//Rzeczywisty rozmiar katalogu
	size_t sizeOnDisk = 9;

	//Dodaje rzeczywisty rozmiar plik�w w katalogu do rozmiaru katalogu
	for (const std::pair<const std::string, File> &file : directory.files) {
		sizeOnDisk += file.second.sizeOnDisk;
	}
	//Przegl�da katalogi i wywo�uje na nich obecn� funkcj� i dodaje zwr�con� warto�� do rozmiaru
	for (const std::pair<const std::string, Directory> &dir : directory.subDirectories) {
		sizeOnDisk += CalculateDirectorySize(dir.second);
	}

	return sizeOnDisk;
}

const unsigned int FileManager::CalculateDirectoryFolderCount(const Directory &directory)
{
	//Ilo�� folder�w w danym katalogu
	unsigned int folderCount = 0;

	//Dodaje ilo�� folder�w w tym folderze do zwracanej zmiennej
	folderCount += directory.subDirectories.size();

	//Przegl�da katalogi i wywo�uje na nich obecn� funkcj� i dodaje zwr�con� warto�� do ilo�ci
	for (const std::pair<const std::string, Directory> &dir : directory.subDirectories) {
		folderCount += CalculateDirectoryFolderCount(dir.second);
	}
	return folderCount;
}

const unsigned int FileManager::CalculateDirectoryFileCount(const Directory &directory)
{
	//Ilo�� plik�w w danym katalogu
	unsigned int filesCount = 0;

	//Dodaje ilo�� plik�w w tym folderze do zwracanej zmiennej
	filesCount += directory.files.size();

	//Przegl�da katalogi i wywo�uje na nich obecn� funkcj� i dodaje zwr�con� warto�� do ilo�ci
	for (const std::pair<const std::string, Directory> &dir : directory.subDirectories) {
		filesCount += CalculateDirectoryFolderCount(dir.second);
	}
	return filesCount;
}

const std::string FileManager::GetCurrentPath() const
{
	//�cie�ka
	std::string path;
	//Tymczasowa zmienna przechowuj�ca wska�nik na katalog
	const Directory* tempDir = currentDirectory;
	//Dop�ki nie doszli�my do pustego katalogu
	while (tempDir != nullptr) {
		//Dodaj do �cie�ki od przodu nazw� obecnego katalogu
		path.insert(0, "/" + tempDir->name);
		//Przypisanie tymczasowej zmiennej katalog wy�szy w hierarchii
		tempDir = tempDir->parentDirectory;
	}
	return path;
}

const std::string FileManager::GetPath(const Directory &directory)
{
	std::string path;
	//Tymczasowa zmienna przechowuj�ca wska�nik na katalog
	const Directory* tempDir = &directory;
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
	Directory* tempDir = currentDirectory;
	//Dop�ki nie doszli�my do pustego katalogu
	while (tempDir != nullptr) {
		//Dodaj do �cie�ki od przodu nazw� obecnego katalogu
		length += tempDir->name.size();
		//Przypisanie tymczasowej zmiennej katalog wy�szy w hierarchii
		tempDir = tempDir->parentDirectory;
	}
	return length;
}

const bool FileManager::CheckIfNameUnused(const Directory &directory, const std::string &name)
{
	//Przeszukuje podany katalog za plikiem o tej samej nazwie
	for (auto i = directory.files.begin(); i != directory.files.end(); ++i) {
		//Je�li nazwa ta sama
		if (i->first == name) { return false; }
	}
	return true;
}

const bool FileManager::CheckIfEnoughSpace(const unsigned int &dataSize) const
{
	//Je�li dane si� mieszcz�
	if (dataSize <= DISK.FileSystem.freeSpace) { return true; }
	//Je�li dane si� nie mieszcz�
	return false;
}

void FileManager::ChangeBitVectorValue(const unsigned int &block, const bool &value) {
	//Je�li warto�� zaj�ty to wolne miejsce - BLOCK_SIZE
	if (value == 1) { DISK.FileSystem.freeSpace -= BLOCK_SIZE; }
	//Je�li warto�� wolny to wolne miejsce + BLOCK_SIZE
	else if (value == 0) { DISK.FileSystem.freeSpace += BLOCK_SIZE; }
	//Przypisanie blokowi podanej warto�ci
	DISK.FileSystem.bitVector[block] = value;
}

void FileManager::WriteFile(const File &file, const std::string &data) {
	//Uzyskuje dane podzielone na fragmenty
	const std::vector<std::string>fileFragments = DataToDataFragments(data);
	//Index pod kt�rym maj� zapisywane by� dane
	unsigned int index = file.FileSystemIndex;

	//Zapisuje wszystkie dane na dysku
	for (const auto& fileFragment : fileFragments)
	{
		//Zapisuje fragment na dysku
		DISK.write(index * BLOCK_SIZE, index * BLOCK_SIZE + fileFragment.size() - 1, fileFragment);
		//Zmienia warto�� bloku w wektorze bitowym na zaj�ty
		ChangeBitVectorValue(index, BLOCK_OCCUPIED);
		//Przypisuje do indeksu numer kolejnego bloku
		index = DISK.FileSystem.FileAllocationTable[index];
	}
}

const std::vector<std::string> FileManager::DataToDataFragments(const std::string &data) const
{
	//Tablica fragment�w podanych danych
	std::vector<std::string>fileFragments;

	//Przetworzenie ca�ych danych
	for (unsigned int i = 0; i < CalculateNeededBlocks(data); i++) {
		//Oblicza pocz�tek kolejnej cz�ci fragmentu danych.
		const unsigned int substrBegin = i * BLOCK_SIZE;
		//Dodaje do tablicy fragment�w kolejny fragment o d�ugo�ci BLOCK_SIZE
		fileFragments.push_back(data.substr(substrBegin, BLOCK_SIZE));
	}
	return fileFragments;
}

const unsigned int FileManager::CalculateNeededBlocks(const std::string &data) const
{
	/*
	Przybli�enie w g�r� rozmiaru pliku przez rozmiar bloku.
	Jest tak, poniewa�, je�li zape�nia chocia� o jeden bajt
	wi�cej przy zaj�tym bloku, to trzeba zaalokowa� wtedy kolejny blok.
	*/
	return int(ceil(double(data.size()) / double(BLOCK_SIZE)));
}

const std::vector<unsigned int> FileManager::FindUnallocatedBlocksFragmented(unsigned int blockCount) {
	//Lista wolnych blok�w
	std::vector<unsigned int> blockList;

	//Szuka wolnych blok�w
	for (unsigned int i = 0; i < DISK.FileSystem.bitVector.size(); i++) {
		//Je�li blok wolny
		if (DISK.FileSystem.bitVector[i] == BLOCK_FREE) {
			//Dodaje indeks bloku
			blockList.push_back(i);
			//Potrzeba teraz jeden blok mniej
			blockCount--;
			//Je�li potrzeba 0 blok�w, przerwij
			if (blockCount == 0) { break; }
		}
	}
	blockList.push_back(-1);
	return blockList;
}

const std::vector<unsigned int> FileManager::FindUnallocatedBlocksBestFit(const unsigned int &blockCount) {
	//Lista indeks�w blok�w (dopasowanie)
	std::vector<unsigned int> blockList;
	//Najlepsze dopasowanie
	std::vector<unsigned int> bestBlockList(DISK.FileSystem.bitVector.size() + 1);

	//Szukanie wolnych blok�w spe�niaj�cych minimum miejsca
	for (unsigned int i = 0; i < DISK.FileSystem.bitVector.size(); i++) {
		//Je�li blok wolny
		if (DISK.FileSystem.bitVector[i] == BLOCK_FREE) {
			//Dodaj indeks bloku do listy blok�w
			blockList.push_back(i);
		}
		//Je�li blok zaj�ty
		else {
			//Je�li uzyskana lista blok�w jest wi�ksza od ilo�ci blok�w jak� chcemy uzyska�
			//to dodaj uzyskane dopasowanie do listy dopasowa�;
			if (blockList.size() >= blockCount) {
				//Je�li znalezione dopasowanie mniejsze ni� najlepsze dopasowanie
				if (blockList.size() < bestBlockList.size()) {
					//Przypisanie nowego najlepszego dopasowania
					bestBlockList = blockList;
					if (bestBlockList.size() == blockCount) { break; }
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
	if (blockList.size() >= blockCount) {
		//Je�li blok wolny
		if (blockList.size() < bestBlockList.size()) {
			//Dodaj indeks bloku do listy blok�w
			bestBlockList = blockList;
		}
	}

	//Je�li znalezione najlepsze dopasowanie
	if (bestBlockList.size() < DISK.FileSystem.bitVector.size() + 1) {
		//Odetnij nadmiarowe indeksy z dopasowania (je�li wi�ksze ni� potrzeba)
		bestBlockList.resize(blockCount);
	}
	//Inaczej zmniejsz dopasowanie do 0, �eby po zwr�ceniu wybrano inn� metod�
	else { bestBlockList.resize(0); }

	return bestBlockList;
}

const std::vector<unsigned int> FileManager::FindUnallocatedBlocks(const unsigned int &blockCount) {
	//Szuka blok�w funkcj� z metod� best-fit
	std::vector<unsigned int> blockList = FindUnallocatedBlocksBestFit(blockCount);

	//Je�li funkcja z metod� best-fit nie znajdzie dopasowa�
	if (blockList.empty()) {
		//Szuka niezaalokowanych blok�w, wybieraj�c pierwsze wolne
		blockList = FindUnallocatedBlocksFragmented(blockCount);
	}

	//Dodaje -1, poniewa� przy zapisie w tablicy FileSystem ostatnia pozycja wskazuje na nic (czyli -1)
	blockList.push_back(-1);
	return blockList;
}

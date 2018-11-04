/**
	SexyOS
	FileManager.h
	Przeznaczenie: Zawiera klasy Disk i FileManager oraz deklaracje metod i konstruktor�w

	@author Tomasz Kilja�czyk
	@version 02/11/18
*/

#ifndef SEXYOS_FILEMANAGER_H
#define SEXYOS_FILEMANAGER_H

#include <ctime>
#include <string>
#include <array>
#include <bitset>
#include <utility>
#include <vector>
#include <unordered_map>
#include <memory>

/*
	To-do:
	- otwieranie i zamykanie pliku (zale�y czy m�j zakres zadania)
	- plik flagi
	- defragmentator (bardzo opcjonalne)
	- zapisywanie plik�w z kodem asemblerowym
	- pliki executable
*/

//Klasa serializera (konwertowanie danych)
class Serializer {
public:
	/**
		Zamienia int na string, gdzie liczba przekonwertowywana
		jest na znaki, aby zajmowa�a mniej miejsca na dysku.

		@param input Liczba jak� chcemy zamieni� na string.
		@return Liczba ca�kowita przekonwertowana na string.
	*/
	static const std::string IntToString(unsigned int input);

	/**
		Zamienia string na int.

		@param input Tekst jaki chcemy zamieni� na liczb�.
		@return Liczba powsta�a po zsumowaniu znak�w.
	*/
	static const unsigned int StringToInt(const std::string& input);
};

//Klasa zarz�dcy przestrzeni� dyskow� i systemem plik�w
class FileManager {
private:
	using u_int = unsigned int;

	//--------------- Definicje sta�ych statycznych -------------
	static const size_t BLOCK_SIZE = 32;	   	    //Rozmiar bloku (bajty)
	static const size_t DISK_CAPACITY = 1024;       //Pojemno�� dysku (bajty)
	static const u_int BLOCK_INDEX_NUMBER = 3;	    //Warto�� oznaczaj�ca d�ugo�� pola blockDirect i blok�w niebezpo�rednich
	static const u_int INODE_NUMBER_LIMIT = 32;     //Maksymalna ilo�� element�w w katalogu
	static const u_int MAX_PATH_LENGTH = 32;        //Maksymalna d�ugo�� �cie�ki
	static const bool BLOCK_FREE = false;           //Warto�� oznaczaj�ca wolny blok
	static const bool BLOCK_OCCUPIED = !BLOCK_FREE; //Warto�� oznaczaj�ca zaj�ty blok
	/**Warto�� oznaczaj�ca ilo�� indeks�w w polu directBlocks*/
	static const u_int BLOCK_DIRECT_INDEX_NUMBER = BLOCK_INDEX_NUMBER - 1;
	/**Maksymalny rozmiar pliku obliczony na podstawie maksymalnej ilo�ci indeks�w*/
	static const size_t MAX_FILE_SIZE = (BLOCK_DIRECT_INDEX_NUMBER + (BLOCK_INDEX_NUMBER - BLOCK_DIRECT_INDEX_NUMBER)*BLOCK_INDEX_NUMBER) * BLOCK_SIZE;



	//--------------------- Definicje sta�ych -------------------



	//---------------- Definicje struktur i klas ----------------

	//Klasa indeksu
	class Index {
	public:
		u_int value; //Warto�� indeksu

		Index() : value(NULL) {}
		explicit Index(const u_int &value) : value(value) {}
		virtual ~Index() = default;
	};

	//Klasa bloku indeksowego
	class IndexBlock : public Index {
	private:
		//Tablica indeks�w/blok�w indeksowych
		std::array<std::shared_ptr<Index>, BLOCK_INDEX_NUMBER> block;
	public:
		IndexBlock() = default;
		virtual ~IndexBlock() = default;

		const u_int size() const { return block.size(); }
		void clear() { std::fill(block.begin(), block.end(), nullptr); }

		std::shared_ptr<Index>& operator [] (const size_t &index);
		const std::shared_ptr<Index>& operator [] (const size_t &index) const;
	};

	//Klasa i-w�z�a
	class Inode {
	public:
		//Podstawowe informacje
		std::string type;

		//Dodatkowe informacje
		tm creationTime;	 //Czas i data utworzenia
		//std::string creator; //Nazwa u�ytkownika, kt�ry utworzy� Inode

		/**
			Konstruktor domy�lny.
		*/
		explicit Inode(std::string type_) : type(std::move(type_)), creationTime() {};
		virtual ~Inode() = default;
	};

	//Klasa pliku dziedzicz�ca po i-w�le
	class File : public Inode {
	public:
		//Podstawowe informacje
		size_t blocksOccupied;   //Ilo�� zajmowanych blok�w
		size_t sizeOnDisk;       //Rozmiar pliku na dysku
		IndexBlock directBlocks; //Bezpo�rednie bloki (na ko�cu 1 blok indeksowy 1-poziomu)

		//Dodatkowe informacje
		tm modificationTime; //Czas i data ostatniej modyfikacji pliku

		/**
			Konstruktor domy�lny.
		*/
		File() : Inode("FILE"), blocksOccupied(0), sizeOnDisk(0), modificationTime() {}

		virtual ~File() = default;
	};

	//Klasa katalogu dziedzicz� po i-w�le
	class Directory : public Inode {
	public:
		std::unordered_map<std::string, std::string> Inodes; //Tablica hashowa Inode
		//std::string parentDirectory; //Wska�nik na katalog nadrz�dny

		/**
			Konstruktor inicjalizuj�cy parentDirectory podan� zmiennymi.

			@param parentDirectory_ Wska�nik na katalog utworzenia
		*/
		explicit Directory(std::string parentDirectory_)
			: Inode("DIRECTORY")/*, parentDirectory(std::move(parentDirectory_))*/ {}
		virtual ~Directory() = default;

		bool operator == (const Directory &dir) const;
	};

	//Prosta klasa dysku (imitacja fizycznego)
	class Disk {
	public:
		struct FileSystem {
			u_int freeSpace{ DISK_CAPACITY }; //Zawiera informacje o ilo�ci wolnego miejsca na dysku (bajty)

			//Wektor bitowy blok�w (0 - wolny blok, 1 - zaj�ty blok)
			std::bitset<DISK_CAPACITY / BLOCK_SIZE> bitVector;

			/**
			 Mapa Inode.
			 */
			std::unordered_map<std::string, std::shared_ptr<Inode>> InodeTable;

			std::string rootDirectory = "root/"; //Katalog g��wny

			/**
				Konstruktor domy�lny. Wpisuje katalog g��wny do tablicy iW�z��w.
			*/
			FileSystem() { InodeTable[rootDirectory] = std::make_shared<Directory>(""); }
		} FileSystem; //System plik�w FileSystem

		//Tablica reprezentuj�ca przestrze� dyskow� (jeden indeks - jeden bajt)
		std::array<char, DISK_CAPACITY> space;

		//----------------------- Konstruktor -----------------------
		/**
			Konstruktor domy�lny. Wykonuje zape�nienie przestrzeni dyskowej warto�ci� NULL.
		*/
		Disk();

		//-------------------------- Metody -------------------------
		/**
			Zapisuje dane (string) na dysku od indeksu 'begin' do indeksu 'end' w��cznie.

			@param begin Indeks od kt�rego dane maj� by� zapisywane.
			@param end Indeks na kt�rym zapisywanie danych ma si� zako�czy�.
			@param data Dane typu string.
			@return void.
		*/
		void write(const u_int &begin, const u_int &end, const std::string &data);

		/**
			Odczytuje dane zadanego typu (je�li jest on zaimplementowany) w wskazanym przedziale.

			@param begin Indeks od kt�rego dane maj� by� odczytywane.
			@param end Indeks do kt�rego dane maj� by� odczytywane.
			@return zmienna zadanego typu.
		*/
		template<typename T>
		const T read(const u_int &begin, const u_int &end) const;
	} DISK;

	//------------------- Definicje zmiennych -------------------
	//u_int fileNumber = 0;  //Licznik plik�w w systemie (katalogi to te� pliki)
	bool messages = false; //Zmienna do w��czania/wy��czania powiadomie�
	bool detailedMessages = false; //Zmienna do w��czania/wy��czania szczeg�owych powiadomie�
	std::string currentDirectory; //Obecnie u�ywany katalog


public:
	//----------------------- Konstruktor -----------------------
	/**
		Konstruktor domy�lny. Przypisuje do obecnego katalogu katalog g��wny.
	*/
	FileManager();



	//-------------------- Podstawowe Metody --------------------
	/**
		Tworzy plik o podanej nazwie i danych w obecnym katalogu.

		@param name Nazwa pliku.
		@param data Dane typu string.
		@return void.
	*/
	bool FileCreate(const std::string &name, const std::string &data);

	//!!!!!!!!!! NIEDOKO�CZONE !!!!!!!!!!
	/**
		Funkcja niedoko�czona.

		@param name Nazwa pliku.
		@return Tymczasowo zwraca dane wczytane z dysku.
	*/
	//const std::string FileOpen(const std::string &name) const;
	//!!!!!!!!!! NIEDOKO�CZONE !!!!!!!!!!


	const std::string FileGetData(const std::string& file);

	bool FileSaveData(const std::string& name, const std::string& data);

	/**
		Usuwa plik o podanej nazwie znajduj�cy si� w obecnym katalogu.
		Plik jest wymazywany z tablicy FileSystem oraz wektora bitowego.

		@param name Nazwa pliku.
		@return void.
	*/
	bool FileDelete(const std::string &name);

	/**
		Tworzy nowy katalog w obecnym katalogu.

		@param name Nazwa katalogu.
		@return void.
	*/
	bool DirectoryCreate(const std::string &name);

	/**
		Usuwa katalog o podanej nazwie.

		@param name Nazwa katalogu.
		@return void.
	*/
	bool DirectoryDelete(const std::string &name);

	/**
		Przechodzi z obecnego katalogu do katalogu nadrz�dnego.

		@return void.
	*/
	bool DirectoryUp();

	/**
		Przechodzi z obecnego katalogu do katalogu podrz�dnego o podanej nazwie

		@param name Nazwa katalogu.
		@return void.
	*/
	bool DirectoryDown(const std::string &name);



	//--------------------- Dodatkowe metody --------------------

	/**
		Zmienia nazw� pliku o podanej nazwie.

		@param name Obecna nazwa pliku.
		@param changeName Zmieniona nazwa pliku.
		@return void.
	*/
	bool FileRename(const std::string &name, const std::string &changeName);

	/**
		Przechodzi z obecnego katalogu do katalogu g��wnego.

		@return void.
	*/
	void DirectoryRoot();

	/**
	Zmienia zmienn� odpowiadaj�c� za wy�wietlanie komunikat�w.
	false - komunikaty wy��czone.
	true - komunikaty w��czone.

	@param onOff Czy komunikaty maj� by� w��czone.
	@return void.
*/
	void Messages(const bool &onOff);

	void DetailedMessages(const bool &onOff);



	//------------------ Metody do wy�wietlania -----------------

	void DisplayFileSystemParams() const;

	/**
		Wy�wietla informacje o wybranym katalogu.

		@return void.
	*/
	bool DisplayDirectoryInfo(const std::string &name);

	/**
		Wy�wietla informacje o pliku.

		@return void.
	*/
	bool DisplayFileInfo(const std::string &name);

	/**
		Wy�wietla struktur� katalog�w.

		@return void.
	*/
	void DisplayDirectoryStructure();
	/**
		Wy�wietla rekurencyjnie katalog i jego podkatalogi.

		@param directory Katalog szczytowy do wy�wietlenia.
		@param level Poziom obecnego katalogu w hierarchii katalog�w.
		@return void.
	*/
	void DisplayDirectory(const std::shared_ptr<Directory>& directory, u_int level);

	/**
		Wy�wietla zawarto�� dysku w formie binarnej.

		@return void.
	*/
	void DisplayDiskContentBinary();

	/**
		Wy�wietla zawarto�� dysku jako znaki.
		'.' - puste pole.

		@return void.
	*/
	void DisplayDiskContentChar();

	/**
		Wy�wietla wektor bitowy.

		@return void.
	*/
	void DisplayBitVector();


private:
	//------------------- Metody Sprawdzaj�ce -------------------

	/**
		Sprawdza czy nazwa pliku jest u�ywana w danym katalogu.

		@param directory Katalog, w kt�rym sprawdzana jest nazwa pliku.
		@param name Nazwa pliku
		@return Prawda, je�li nazwa nieu�ywana, inaczej fa�sz.
	*/
	const bool CheckIfNameUnused(const std::string& directory, const std::string& name);

	/**
		Sprawdza czy jest miejsce na dane o zadaniej wielko�ci.

		@param dataSize Rozmiar danych, dla kt�rych b�dziemy sprawdzac miejsce.
		@return void.
	*/
	const bool CheckIfEnoughSpace(const u_int& dataSize) const;



	//-------------------- Metody Pomocnicze --------------------

	const std::string GetCurrentDirectoryParent() const;

	/**
		Wczytuje dane pliku z dysku.

		@param file Plik, kt�rego dane maj� by� wczytane.
		@return Dane pliku w postaci string.
	*/
	const std::string FileGetData(const std::shared_ptr<File>& file) const;

	void FileAddIndexes(const std::shared_ptr<File>& file, const std::vector<u_int> &blocks) const;

	void FileAllocateBlocks(const std::shared_ptr<File>& file, const std::vector<u_int>& blocks);

	void FileAllocationIncrease(std::shared_ptr<File>& file, const u_int &neededBlocks);

	void FileAllocationDecrease(const std::shared_ptr<File>& file, const u_int &neededBlocks);

	void FileDeallocate(const std::shared_ptr<File>& file);

	/**
		Usuwa ca�� struktur� katalog�w.

		@param directory Katalog do usuni�cia.
		@return Rozmiar podanego katalogu.
	*/
	void DirectoryDeleteStructure(std::shared_ptr<Directory>& directory);

	/**
		Usuwa wskazany plik.

		@param file Plik do usuni�cia.
		@return void.
	*/
	void FileDelete(std::shared_ptr<File>& file);

	/**
		Zwraca rozmiar podanego katalogu.

		@return Rozmiar podanego katalogu.
	*/
	const size_t CalculateDirectorySize(const std::shared_ptr<Directory>& directory);

	/**
		Zwraca rzeczywisty rozmiar podanego katalogu.

		@return Rzeczywisty rozmiar podanego katalogu.
	*/
	const size_t CalculateDirectorySizeOnDisk(const std::shared_ptr<Directory>& directory);

	/**
		Zwraca liczb� folder�w (katalog�w) w danym katalogu i podkatalogach.

		@return Liczba folder�w.
	*/
	const u_int CalculateDirectoryFolderNumber(const std::shared_ptr<Directory>& directory);

	/**
		Zwraca liczb� plik�w w danym katalogu i podkatalogach.

		@return Liczba plik�w.
	*/
	const u_int CalculateDirectoryFileNumber(const std::shared_ptr<Directory>& directory);

	/**
		Zwraca �cie�k� przekazanego folderu

		@param directory Katalog, kt�rego �ci�k� chcemy otrzyma�.
		@return Obecna �cie�ka z odpowiednim formatowaniem.
	*/
	const std::string GetPath(const std::shared_ptr<Directory>& directory);

	/**
		Zwraca obecnie u�ywan� �cie�k�.

		@return Obecna �cie�ka z odpowiednim formatowaniem.
	*/
	const std::string GetCurrentPath() const;

	/**
		Zwraca d�ugo�� obecnej �cie�ki.

		@return d�ugo�� obecnej �cie�ki.
	*/
	const size_t GetCurrentPathLength() const;

	/**
		Zwraca aktualny czas i dat�.

		@return Czas i data.
	*/
	static const tm GetCurrentTimeAndDate();

	/**
		Zmienia warto�� w wektorze bitowym i zarz� pole freeSpace
		w strukturze FileSystem.

		@param block Indeks bloku, kt�rego warto�� w wektorze bitowym b�dzie zmieniana.
		@param value Warto�� do przypisania do wskazanego bloku (0 - wolny, 1 - zaj�ty)
		@return void.
	*/
	void ChangeBitVectorValue(const u_int &block, const bool &value);


	/**
		Zmniejsza plik do podanego rozmiaru. Podany rozmiar
		musi by� mniejszy od rozmiaru pliku o conajmniej jedn�
		jednostk� alokacji

		@param file Wska�nik na plik, kt�rego rozmiar chcemy zmieni�.
		@param neededBlocks Ilo�� blok�w do alokacji.
		@return void.
	*/
	void FileTruncate(std::shared_ptr<File> file, const u_int &neededBlocks);

	/**
		Zapisuje wektor fragment�w File.data na dysku.

		@param file Plik, kt�rego dane b�d� zapisane na dysku.
		@param data Dane do zapisania na dysku.
		@return void.
	*/
	void FileSaveData(std::shared_ptr<File> &file, const std::string &data);

	/**
		Dzieli string na fragmenty o rozmiarze BLOCK_SIZE.

		@param data String do podzielenia na fradmenty.
		@return Wektor fragment�w string.
	*/
	const std::vector<std::string> DataToDataFragments(const std::string &data) const;

	/**
		Oblicza ile blok�w zajmie podany string.

		@param dataSize D�ugo�� danych, kt�rych rozmiar na dysku b�dzie obliczany.
		@return Ilo�� blok�w jak� zajmie string.
	*/
	const u_int CalculateNeededBlocks(const size_t &dataSize) const;

	/**
		Znajduje nieu�ywane bloki do zapisania pliku bez dopasowania do luk w blokach

		@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Wektor indeks�w blok�w do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocksFragmented(u_int blockNumber);

	/*
		Znajduje nieu�ywane bloki do zapisania pliku metod� best-fit.

		@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Wektor indeks�w blok�w do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocksBestFit(const u_int &blockNumber);

	/*
		Znajduje nieu�ywane bloki do zapisania pliku. Najpierw uruchamia funkcj�
		dzia�aj�c� metod� best-fit, je�li funkcja nie znajdzie dopasowania do
		uruchamia funkcj� znajduj�c� pierwsze jakiekolwiek wolne bloki i wprowadza
		fragmentacj� danych.

		@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Wektor indeks�w blok�w do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocks(const u_int &blockNumber);

};

static FileManager fileManager;

#endif //SEXYOS_FILEMANAGER_H

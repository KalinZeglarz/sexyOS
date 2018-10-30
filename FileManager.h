/**
	SexyOS
	FileManager.h
	Przeznaczenie: Zawiera klasy Disk i FileManager oraz deklaracje metod i konstruktor�w

	@author Tomasz Kilja�czyk
	@version 30/10/18
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
	- iNode
	- otwieranie i zamykanie pliku
	- plik flagi
	- defragmentator
	- zapisywanie plik�w z kodem asemblerowym
	- pliki executable
*/

//Klasa zarz�dcy przestrzeni� dyskow� i systemem plik�w
class FileManager {
	using u_int = unsigned int;

private:
	//--------------- Definicje sta�ych statycznych -------------
	static const u_int BLOCK_SIZE = 32;				//Rozmiar bloku (bajty)
	static const size_t DISK_CAPACITY = 1024;       //Pojemno�� dysku (bajty)
	static const size_t BLOCK_INDEX_COUNT = 3;		//Warto�� oznaczaj�ca d�ugo�� pola blockDirect i blok�w niebezpo�rednich
	static const size_t BLOCK_DIRECT_INDEX = 2;     //Warto�� oznaczaj�ca ilo�� indeks�w w polu directBlocks
	static const bool BLOCK_FREE = false;           //Warto�� oznaczaj�ca wolny blok
	static const bool BLOCK_OCCUPIED = !BLOCK_FREE; //Warto�� oznaczaj�ca zaj�ty blok

	/*
	 * Z oblicze� wynika, �e maksymalny rozmiar pliku to 160 bajt�w/znak�w
	 */

	//--------------------- Definicje sta�ych -------------------
	const size_t MAX_PATH_LENGTH = 32;   //Maksymalna d�ugo�� �cie�ki
	const size_t MAX_DIRECTORY_ELEMENTS = 8; //Maksymalna ilo�� element�w w katalogu

	//---------------- Definicje struktur i klas ----------------

	class Index
	{
	public:
		u_int value; //Warto�� indeksu

		Index() : value(NULL) {}
		explicit Index(const u_int &value) : value(value){}
		virtual ~Index(){}
	};
	class IndexBlock : public Index {
	public:
		//Tablica indeks�w/blok�w indeksowych
		std::array<std::shared_ptr<Index>, BLOCK_INDEX_COUNT> value;

		IndexBlock(){}
		virtual ~IndexBlock() { std::fill(value.begin(), value.end(), nullptr); }

		std::shared_ptr<Index>& operator [] (const size_t &index);
		const std::shared_ptr<Index>& operator [] (const size_t &index) const;
	};

	//Struktura pliku
	struct File {
		//Podstawowe informacje
		std::string name;  //Nazwa pliku
		size_t size;	   //Rozmiar pliku
		size_t sizeOnDisk; //Rozmiar pliku na dysku
		/**
		Tablica blok�w bezpo�rednich.
		Dwa ostatnie pola s� zarezerwowane na tablice
		blok�w po�rednich 1 i 2 poziomowych.
		 */
		IndexBlock directBlocks; //Bezpo�rednie bloki (w tym blok indeksowy 1-poziomu)
		//u_int FileSystemIndex;  //Indeks pozycji pocz�tku pliku w tablicy FAT --- ZB�DNE

		//Dodatkowe informacje
		tm creationTime;	 //Czas i data utworzenia pliku
		tm modificationTime; //Czas i data ostatniej modyfikacji pliku
		std::string creator; //Nazwa u�ytkownika, kt�ry utworzy� plik

		/**
			Konstruktor domy�lny.
		*/
		File(): size(0), sizeOnDisk(0), creationTime(), modificationTime(){}

		/**
			Konstruktor inicjalizuj�cy pola name podanymi zmiennymi.

			@param name_ Nazwa pliku.
		*/
		explicit File(const std::string &name_) : name(name_), size(0), sizeOnDisk(0),
												  creationTime(), modificationTime(){}
	};

	//Struktura katalogu
	struct Directory {
		std::string name;  //Nazwa katalogu
		tm creationTime;   //Czas i data utworzenia katalogu

		std::unordered_map<std::string, File> files; //Tablica hashowa plik�w w katalogu
		std::unordered_map<std::string, Directory>subDirectories; //Tablica hashowa podkatalog�w
		Directory* parentDirectory; //Wska�nik na katalog nadrz�dny

		/**
			Konstruktor domy�lny.
		*/
		Directory(): creationTime(), parentDirectory(nullptr){}

		/**
			Konstruktor inicjalizuj�cy pole name i parentDirectory podanymi zmiennymi.

			@param name_ Nazwa pliku.
			@param parentDirectory_ Wska�nik na katalog utworzenia
		*/
		Directory(std::string name_, Directory* parentDirectory_) : name(std::move(name_)), creationTime(),
		                                                                   parentDirectory(parentDirectory_){}
	};

	class Disk {
	public:
		struct FileSystem {
			u_int freeSpace{ DISK_CAPACITY }; //Zawiera informacje o ilo�ci wolnego miejsca na dysku (bajty)

			//Wektor bitowy blok�w (0 - wolny blok, 1 - zaj�ty blok)
			std::bitset<DISK_CAPACITY / BLOCK_SIZE> bitVector;

			Directory rootDirectory{ Directory("root", nullptr) }; //Katalog g��wny

			/**
				Konstruktor domy�lny. Wykonuje zape�nienie tablicy FileSystem warto�ci� -1.
			*/
			FileSystem(){}
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
			Zapisuje dane (u_int) pod wskazanym indeksem.

			@param index Indeks na kt�rym zapisana zostanie liczba.
			@param data Liczba typu u_int.
			@return void.
		*/
		void write(const u_int &index, const u_int &data);

		/**
			Odczytuje dane zadanego typu (je�li jest on zaimplementowany) w wskazanym przedziale.

			@param begin Indeks od kt�rego dane maj� by� odczytywane.
			@param end Indeks do kt�rego dane maj� by� odczytywane.
			@return zmienna zadanego typu.
		*/
		template<typename T>
		const T read(const u_int &begin, const u_int &end);
	} DISK; //Prosta klasa dysku (imitacja fizycznego)

	//------------------- Definicje zmiennych -------------------
	bool messages = false; //Zmienna do w��czania/wy��czania powiadomie�
	Directory* currentDirectory; //Obecnie u�ytkowany katalog

public:
	//----------------------- Konstruktor -----------------------
	/**
		Konstruktor domy�lny. Przypisuje do obecnego katalogu katalog g��wny.
	*/
	FileManager();

	//-------------------- Podstawowe Metody --------------------
	/**
		Tworzy plik o podanej nazwie i danych w obecnym katalogu.

		@param name Nazwa pliku
		@param data Dane typu string.
		@return void.
	*/
	void FileCreate(const std::string &name, const std::string &data);

	//!!!!!!!!!! NIEDOKO�CZONE !!!!!!!!!!
	/**
		Funkcja niedoko�czona.

		@param name Nazwa pliku.
		@return Tymczasowo zwraca dane wczytane z dysku.
	*/
	const std::string FileOpen(const std::string &name);
	//!!!!!!!!!! NIEDOKO�CZONE !!!!!!!!!!

	/**
		Wczytuje dane pliku z dysku.

		@param file Plik, kt�rego dane maj� by� wczytane.
		@return Dane pliku w postaci string.
	*/
	const std::string FileGetData(const File &file);

	/**
		Usuwa plik o podanej nazwie znajduj�cy si� w obecnym katalogu.
		Plik jest wymazywany z tablicy FileSystem oraz wektora bitowego.

		@param name Nazwa pliku.
		@return void.
	*/
	void FileDelete(const std::string &name);

	/**
		Zmniejsza plik do podanego rozmiaru. Podany rozmiar
		musi by� mniejszy od rozmiaru pliku o conajmniej jedn�
		jednostk� alokacji

		@param name Nazwa pliku.
		@param size Rozmiar do kt�rego plik ma by� zmniejszony.
		@return void.
	*/
	void FileTruncate(const std::string &name, const u_int &size);

	/**
		Tworzy nowy katalog w obecnym katalogu.

		@param name Nazwa katalogu.
		@return void.
	*/
	void DirectoryCreate(const std::string &name) const;

	/**
		Usuwa katalog o podanej nazwie.

		@param name Nazwa katalogu.
		@return void.
	*/
	void DirectoryDelete(const std::string &name);


	/**
		Przechodzi z obecnego katalogu do katalogu nadrz�dnego.

		@return void.
	*/
	void DirectoryUp();

	/**
		Przechodzi z obecnego katalogu do katalogu podrz�dnego o podanej nazwie

		@param name Nazwa katalogu.
		@return void.
	*/
	void DirectoryDown(const std::string &name);

	//--------------------- Dodatkowe metody --------------------
	/**
		Zmienia nazw� pliku o podanej nazwie.

		@param name Obecna nazwa pliku.
		@param changeName Zmieniona nazwa pliku.
		@return void.
	*/
	void FileRename(const std::string &name, const std::string &changeName) const;
	
	/**
		Przechodzi z obecnego katalogu do katalogu g��wnego.

		@return void.
	*/
	void DirectoryRoot();

	//------------------ Metody do wy�wietlania -----------------
	/**
		Zmienia zmienn� odpowiadaj�c� za wy�wietlanie komunikat�w.
		false - komunikaty wy��czone.
		true - komunikaty w��czone.

		@param onOff Czy komunikaty maj� by� w��czone.
		@return void.
	*/
	void Messages(const bool &onOff);

	/**
		Wy�wietla informacje o wybranym katalogu.

		@return void.
	*/
	void DisplayDirectoryInfo(const std::string &name) const;

	/**
		Wy�wietla informacje o pliku.

		@return void.
	*/
	void DisplayFileInfo(const std::string &name);

	/**
		Wy�wietla struktur� katalog�w.

		@return void.
	*/
	void DisplayDirectoryStructure() const;
	/**
		Wy�wietla rekurencyjnie katalog i jego podkatalogi.

		@param directory Katalog szczytowy do wy�wietlenia.
		@param level Poziom obecnego katalogu w hierarchii katalog�w.
		@return void.
	*/
	static void DisplayDirectory(const Directory &directory, u_int level);

	/**
		Wy�wietla zawarto�� dysku w formie binarnej.

		@return void.
	*/
	void DisplayDiskContentBinary();

	/**
		Wy�wietla zawarto�� dysku w znak�w.

		@return void.
	*/
	void DisplayDiskContentChar();

	//**
	//	Wy�wietla tablic� alokacji plik�w (FileSystem).
	//
	//	@return void.
	//*/
	//void DisplayFileAllocationTable();

	/**
		Wy�wietla wektor bitowy.

		@return void.
	*/
	void DisplayBitVector();

	/**
		Wy�wietla plik podzielony na fragmenty.

		@return void.
	*/
	static void DisplayFileFragments(const std::vector<std::string> &fileFragments);

private:
	//-------------------- Metody Pomocnicze --------------------
	/**
		Usuwa ca�� struktur� katalog�w.

		@param directory Katalog do usuni�cia.
		@return Rozmiar podanego katalogu.
	*/
	void DirectoryDeleteStructure(Directory &directory);

	/**
		Usuwa wskazany plik.

		@param file Plik do usuni�cia.
		@return void.
	*/
	void FileDelete(File &file);

	/**
		Zwraca rozmiar podanego katalogu.

		@return Rozmiar podanego katalogu.
	*/
	static const size_t CalculateDirectorySize(const Directory &directory);

	/**
		Zwraca rzeczywisty rozmiar podanego katalogu.

		@return Rzeczywisty rozmiar podanego katalogu.
	*/
	static const size_t CalculateDirectorySizeOnDisk(const Directory &directory);

	/**
		Zwraca liczb� folder�w (katalog�w) w danym katalogu i podkatalogach.

		@return Liczba folder�w.
	*/
	static const u_int CalculateDirectoryFolderCount(const Directory &directory);

	/**
		Zwraca liczb� plik�w w danym katalogu i podkatalogach.

		@return Liczba plik�w.
	*/
	static const u_int CalculateDirectoryFileCount(const Directory &directory);

	/**
		Zwraca �cie�k� przekazanego folderu

		@param directory Katalog, kt�rego �ci�k� chcemy otrzyma�.
		@return Obecna �cie�ka z odpowiednim formatowaniem.
	*/
	static const std::string GetPath(const Directory &directory);

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
		Sprawdza czy nazwa pliku jest u�ywana w danym katalogu.

		@param directory Katalog, w kt�rym sprawdzana jest nazwa pliku.
		@param name Nazwa pliku
		@return Prawda, je�li nazwa nieu�ywana, inaczej fa�sz.
	*/
	static const bool CheckIfNameUnused(const Directory &directory, const std::string &name);

	/**
		Sprawdza czy jest miejsce na dane o zadaniej wielko�ci.

		@param dataSize Rozmiar danych, dla kt�rych b�dziemy sprawdzac miejsce.
		@return void.
	*/
	const bool CheckIfEnoughSpace(const u_int &dataSize) const;

	/**
		Zmienia warto�� w wektorze bitowym i zarz� pole freeSpace
		w strukturze FileSystem.

		@param block Indeks bloku, kt�rego warto�� w wektorze bitowym b�dzie zmieniana.
		@param value Warto�� do przypisania do wskazanego bloku (0 - wolny, 1 - zaj�ty)
		@return void.
	*/
	void ChangeBitVectorValue(const u_int &block, const bool &value);

	/**
		Zapisuje wektor fragment�w File.data na dysku.

		@param file Plik, kt�rego dane b�d� zapisane na dysku.
		@param data Dane do zapisania na dysku.
		@return void.
	*/
	void WriteFile(const File &file, const std::string &data);

	/**
		Dzieli string na fragmenty o rozmiarze BLOCK_SIZE.

		@param data String do podzielenia na fradmenty.
		@return Wektor fragment�w string.
	*/
	const std::vector<std::string> DataToDataFragments(const std::string &data) const;

	/**
		Oblicza ile blok�w zajmie podany string.

		@param data String, kt�rego rozmiar na dysku, b�dzie obliczany.
		@return Ilo�� blok�w jak� zajmie string.
	*/
	const u_int CalculateNeededBlocks(const std::string &data) const;

	/**
		Znajduje nieu�ywane bloki do zapisania pliku bez dopasowania do luk w blokach

		@param blockCount Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Wektor indeks�w blok�w do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocksFragmented(u_int blockCount);

	/*
		Znajduje nieu�ywane bloki do zapisania pliku metod� best-fit.

		@param blockCount Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Wektor indeks�w blok�w do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocksBestFit(const u_int &blockCount);

	/*
		Znajduje nieu�ywane bloki do zapisania pliku. Najpierw uruchamia funkcj�
		dzia�aj�c� metod� best-fit, je�li funkcja nie znajdzie dopasowania do
		uruchamia funkcj� znajduj�c� pierwsze jakiekolwiek wolne bloki i wprowadza
		fragmentacj� danych.

		@param blockCount Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Wektor indeks�w blok�w do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocks(const u_int &blockCount);

};

static FileManager fileManager;

#endif //SEXYOS_FILEMANAGER_H

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
	- zabezpieczenia + flagi w plikach
	- otwieranie i zamykanie pliku
	- zapisywanie plik�w z kodem asemblerowym (opcjonalne)
	- pliki executable (opcjonalne)
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
		Zamienia string na int. Do u�ycia w przypadku odczytywania liczby z dysku.

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



	//---------------- Definicje struktur i klas ----------------

	//Klasa indeksu - przechowuje indeks bloku dyskowego.
	class Index {
	public:
		u_int value; //Warto�� indeksu

		Index() : value(NULL) {}
		explicit Index(const u_int& value) : value(value) {}
		virtual ~Index() = default;
	};

	//Klasa bloku indeksowego - przechowuje tablic� indeks�w.
	class IndexBlock : public Index {
	private:
		//Tablica indeks�w/blok�w indeksowych
		std::array<std::shared_ptr<Index>, BLOCK_INDEX_NUMBER> block;
	public:
		IndexBlock() = default;
		virtual ~IndexBlock() = default;

		const u_int size() const { return block.size(); }
		void clear() { std::fill(block.begin(), block.end(), nullptr); }

		std::shared_ptr<Index>& operator [] (const size_t& index);
		const std::shared_ptr<Index>& operator [] (const size_t& index) const;
	};

	//Klasa i-w�z�a - zawiera podstawowe informacje o pliku
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
		explicit Inode(std::string type_) : type(std::move(type_)), creationTime(FileManager::GetCurrentTimeAndDate()) {}
		virtual ~Inode() = default;
	};

	//Klasa pliku dziedzicz�ca po i-w�le - zawiera informacje specyficzne dla pliku
	class File : public Inode {
	public:
		//Podstawowe informacje
		size_t blocksOccupied;   //Ilo�� zajmowanych blok�w
		size_t sizeOnDisk;       //Rozmiar pliku na dysku
		IndexBlock directBlocks; //Bezpo�rednie bloki (na ko�cu 1 blok indeksowy 1-poziomu)

		//Dodatkowe informacje
		std::string creator;
		tm modificationTime; //Czas i data ostatniej modyfikacji pliku

		/**
			Flagi znaczenie:

			indeks 0 - flaga plik otwarty

			indeks 1 - flaga odczytu

			indeks 2 - flaga zapisu

			Domy�lnie plik jest zamkni�ty i ma ustawione prawa odczytu i zapisu.
		 */
		std::bitset<3> flags;

		/**
			Konstruktor domy�lny.
		*/
		File() : Inode("FILE"), blocksOccupied(0), sizeOnDisk(0), modificationTime(creationTime) {}

		virtual ~File() = default;
	};

	//Klasa katalogu dziedzicz� po i-w�le - zawiera informacje specyficzne dla katalogu
	class Directory : public Inode {
	public:
		std::unordered_map<std::string, std::string> files; //Tablica hashowa Inode

		/**
			Konstruktor inicjalizuj�cy nadrz�dny Inode typem "DIRECTORY".
		*/
		explicit Directory() : Inode("DIRECTORY") {}
		virtual ~Directory() = default;
	};

	//Prosta klasa dysku (imitacja fizycznego) - przechowuje dane + system plik�w
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
			FileSystem();
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
		void write(const u_int& begin, const std::string& data);

		/**
			Odczytuje dane zadanego typu (je�li jest on zaimplementowany) w wskazanym przedziale.

			@param begin Indeks od kt�rego dane maj� by� odczytywane.
			@param end Indeks do kt�rego dane maj� by� odczytywane.
			@return zmienna zadanego typu.
		*/
		template<typename T>
		const T read(const u_int& begin) const;
	} DISK;



	//------------------- Definicje zmiennych -------------------
	bool messages = false; //Zmienna do w��czania/wy��czania powiadomie�
	bool detailedMessages = false; //Zmienna do w��czania/wy��czania szczeg�owych powiadomie�
	std::string currentDirectory; //Obecnie u�ywany katalog
	std::unordered_map<std::string, u_int> usedFiles;


public:
	//----------------------- Konstruktor -----------------------
	/**
		Konstruktor domy�lny. Przypisuje do obecnego katalogu katalog g��wny.
	*/
	FileManager();



	//-------------------- Podstawowe Metody --------------------
	/**
		Tworzy plik o podanej nazwie w obecnym katalogu.

		@param name Nazwa pliku.
		@return True, je�li operacja si� uda�a i false, je�li operacja nie powiod�a si�.
	*/
	bool FileCreate(const std::string& name);

	/**
		Zapisuje podane dane w danym pliku.

		@param name Nazwa pliku.
		@param data Dane do zapisu.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool FileWriteData(const std::string& name, const std::string& data);

	/**
		Odczytuje dane z podanego pliku.

		@param name Nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	const std::string FileReadData(const std::string& name);

	/**
		Usuwa plik o podanej nazwie znajduj�cy si� w obecnym katalogu.
		Plik jest wymazywany z tablicy i-w�z��w oraz wektora bitowego.

		@param name Nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool FileDelete(const std::string& name);

	bool FileOpen(const std::string& name);

<<<<<<< HEAD
	bool FileClose(const std::string& path);
=======
	bool FileClose(const std::string& name);
>>>>>>> 3b567d58d87389659a9092de5f02974fba4e26d7

	/**
		Ustawia zestaw flag w pliku o podanej nazwie dla podanego u�ytkownika.

		@param name Nazwa pliku.
		@param user U�ytkownik do jakiego chcemy przypisa� flagi.
		@param read Flaga odczytu.
		@param write Flaga zapisu.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool FileSetFlags(const std::string& name, const std::string& user, const bool& read, const bool& write);

	/**
		Tworzy nowy katalog w obecnym katalogu.

		@param name Nazwa katalogu.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool DirectoryCreate(std::string name);

	/**
		Usuwa katalog o podanej nazwie.

		@param name Nazwa katalogu.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool DirectoryDelete(std::string name);

	/**
		Przechodzi z obecnego katalogu wskazanego katalogu.

		@param path �cie�ka katalogu, do kt�rego chcemy przej��.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool DirectoryChange(std::string path);

	/**
		Przechodzi z obecnego katalogu do katalogu nadrz�dnego.

		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool DirectoryUp();

	/**
		Przechodzi z obecnego katalogu do katalogu podrz�dnego o podanej nazwie.

		@param name Nazwa katalogu.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool DirectoryDown(std::string name);



	//--------------------- Dodatkowe metody --------------------

	/**
		Formatuje dysk. Zeruje wektor bitowy, usuwa wszystkie i-w�z�y,
		tworzy nowy katalog g��wny.

		@return void.
	*/
	void DiskFormat();

	/**
		Tworzy plik o podanej nazwie w obecnym katalogu i zapisuje w nim podane dane.

		@param name Nazwa pliku.
		@param data Dane typu string.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool FileCreate(const std::string& name, const std::string& data);

	bool FilePIDSet(const std::string& name, const u_int& pid);

	u_int FilePIDGet(const std::string& path);

	/**
		Zmienia nazw� katalogu (w obecnej �cie�ce) o podanej nazwie.

		@param name Obecna nazwa pliku.
		@param changeName Zmieniona nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool FileRename(const std::string& name, const std::string& changeName);

	/**
		Zmienia nazw� pliku (w obecnej �cie�ce) o podanej nazwie.

		@param name Obecna nazwa pliku.
		@param changeName Zmieniona nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool DirectoryRename(std::string name, std::string changeName);

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
	void Messages(const bool& onOff);

	/**
		Zmienia zmienn� odpowiadaj�c� za wy�wietlanie szczeg�owych komunikat�w.
		false - komunikaty wy��czone.
		true - komunikaty w��czone.

		@param onOff Czy komunikaty maj� by� w��czone.
		@return void.
	*/
	void DetailedMessages(const bool& onOff);

	/**
		Zwraca obecnie u�ywan� �cie�k�.

		@return Obecna �cie�ka z odpowiednim formatowaniem.
	*/
	const std::string GetCurrentPath() const;



	//------------------ Metody do wy�wietlania -----------------

	/**
		Wy�wietla parametry systemu plik�w.

		@return void.
	*/
	void DisplayFileSystemParams() const;

	/**
		Wy�wietla informacje o wybranym katalogu.

		@return void.
	*/
	bool DisplayDirectoryInfo(std::string name);

	/**
		Wy�wietla informacje o pliku.

		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool DisplayFileInfo(const std::string& name);

	/**
		Wy�wietla struktur� katalog�w.

		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	void DisplayDirectoryStructure();

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
		@return Prawda, je�li nazwa u�ywana, inaczej fa�sz.
	*/
	const bool CheckIfNameUsed(const std::string& directory, const std::string& name);

	/**
		Sprawdza czy jest miejsce na dane o zadaniej wielko�ci.

		@param dataSize Rozmiar danych, dla kt�rych b�dziemy sprawdzac miejsce.
		@return void.
	*/
	const bool CheckIfEnoughSpace(const u_int& dataSize) const;



	//-------------------- Metody Obliczaj�ce -------------------

	/**
		Oblicza ile blok�w zajmie podany string.

		@param dataSize D�ugo�� danych, kt�rych rozmiar na dysku b�dzie obliczany.
		@return Ilo�� blok�w jak� zajmie string.
	*/
	const u_int CalculateNeededBlocks(const size_t& dataSize) const;

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



	//--------------------- Metody Alokacji ---------------------

	/**
		Zmniejsza lub zwi�ksza plik do podanego rozmiaru.

		@param file Wska�nik na plik, kt�rego rozmiar chcemy zmieni�.
		@param neededBlocks Ilo�� blok�w do alokacji.
		@return void.
	*/
	void FileTruncate(std::shared_ptr<File> file, const u_int& neededBlocks);

	/**
		Dodaje do pliku podane indeksy blok�w.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@param blocks Numery blok�w do alokacji.
		@return void.
	*/
	void FileAddIndexes(const std::shared_ptr<File>& file, const std::vector<u_int>& blocks) const;

	/**
		Przeprowadza dealokacje danych pliku, czyli usuwa z pliku indeksy blok�w
		oraz zmienia warto�ci w wektorze bitowym.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@return void.
	*/
	void FileDeallocate(const std::shared_ptr<File>& file);

	/**
		Alokuje przestrze� na podane bloki. Zmienia warto�ci w wektorze bitowym,
		aktualizuje warto�� zajmowanych blok�w przez plik oraz wywo�uje funkcj�
		FileAddIndexes.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@param blocks Numery blok�w do alokacji.
		@return void.
	*/
	void FileAllocateBlocks(const std::shared_ptr<File>& file, const std::vector<u_int>& blocks);

	/**
		Obs�uguje proces zwi�kszania liczby zaalokowanych blok�w na dane pliku.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@param neededBlocks Liczba blok�w do alokacji.
		@return void.
	*/
	void FileAllocationIncrease(std::shared_ptr<File>& file, const u_int& neededBlocks);

	/**
		Obs�uguje proces zwi�kszania liczby zaalokowanych blok�w na dane pliku.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@param neededBlocks Liczba blok�w do alokacji.
		@return void.
	*/
	void FileAllocationDecrease(const std::shared_ptr<File>& file, const u_int& neededBlocks);

	/**
	Znajduje nieu�ywane bloki do zapisania pliku bez dopasowania do luk w blokach

	@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
	@return Zestaw indeks�w blok�w mo�liwych do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocksFragmented(u_int blockNumber);

	/*
		Znajduje nieu�ywane bloki do zapisania pliku metod� best-fit.

		@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Zestaw indeks�w blok�w mo�liwych do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocksBestFit(const u_int& blockNumber);

	/*
		Znajduje nieu�ywane bloki do zapisania pliku. Najpierw uruchamia funkcj�
		dzia�aj�c� metod� best-fit, je�li funkcja nie znajdzie dopasowania to
		uruchamiana jest funkcja znajduj�c� pierwsze jakiekolwiek wolne bloki i
		wprowadzaj�ca fragmentacj� danych.

		@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Zestaw indeks�w blok�w mo�liwych do zaalokowania.
	*/
	const std::vector<u_int> FindUnallocatedBlocks(const u_int& blockNumber);



	//----------------------- Metody Inne -----------------------

	/**
		Zapisuje wektor fragment�w File.data na dysku.

		@param file Wska�nik na plik kt�rego dane b�d� zapisane na dysku.
		@param data Dane do zapisania na dysku.
		@return void.
	*/
	void FileWriteData(std::shared_ptr<File>& file, const std::string& data);

	/**
		Wczytuje dane pliku z dysku.

		@param file Wska�nik na plik kt�rego dane maj� by� wczytane z dysku.
		@return Dane pliku w postaci string.
	*/
	const std::string FileReadData(const std::shared_ptr<File>& file) const;

	/**
		Usuwa wskazany plik.

		@param file Wska�nik pliku do usuni�cia.
		@return void.
	*/
	void FileDelete(std::shared_ptr<File>& file);

	/**
		Usuwa ca�� struktur� katalog�w.

		@param directory Katalog do usuni�cia.
		@return Rozmiar podanego katalogu.
	*/
	void DirectoryDeleteStructure(std::shared_ptr<Directory>& directory);

	/**
		Aktualizuje �cie�ki w ca�ej strukturze katalog�w.

		@param directory Wska�nik na katalog, kt�rego �cie�ki chcemy zauktualizowa�.
		@return Rozmiar podanego katalogu.
	*/
	void DirectoryRenameStructure(std::shared_ptr<Directory>& directory);

	/**
		Wy�wietla rekurencyjnie katalog i jego podkatalogi.

		@param directory Katalog szczytowy do wy�wietlenia.
		@param level Poziom obecnego katalogu w hierarchii katalog�w.
		@return void.
	*/
	void DisplayDirectory(const std::shared_ptr<Directory>& directory, u_int level);

	/**
		Usuwa i-w�ze� z tablicy i-w�z��w.

		@param inode Wska�nik na i-w�ze� do usuni�cia.
		@return void.
	*/
	void InodeTableRemove(const std::shared_ptr<Inode>& inode);

	/**
		Zwraca �cie�k� podanego katalogu.

		@param directory Wska�nik na katalog kt�rego �ci�k� chcemy otrzyma�.
		@return �cie�ka podanego katalogu.
	*/
	const std::string GetPath(const std::shared_ptr<Directory>& directory);

	/**
		Zwraca �cie�k� katologu nadrz�dnego wzgl�dem obecnego katalogu.

		@return �cie�ka katalogu nadrz�dnego.
	*/
	const std::string GetCurrentDirectoryParent() const;

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
		Zmienia warto�� w wektorze bitowym i zarz�dza polem freeSpace
		w strukturze FileSystem.

		@param block Indeks bloku, kt�rego warto�� w wektorze bitowym b�dzie zmieniona.
		@param value Warto�� do przypisania do wskazanego bloku ( BLOCK_FREE lub BLOCK_OCCUPIED)
		@return void.
	*/
	void ChangeBitVectorValue(const u_int& block, const bool& value);

	/**
		Dzieli string na fragmenty o rozmiarze BLOCK_SIZE.

		@param data String do podzielenia na fragmenty.
		@return Wektor fragment�w string.
	*/
	const std::vector<std::string> DataToDataFragments(const std::string& data) const;
};

static FileManager fileManager;

#endif //SEXYOS_FILEMANAGER_H

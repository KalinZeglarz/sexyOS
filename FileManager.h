/**
	SexyOS
	FileManager.h
	Przeznaczenie: Zawiera klasy Disk i FileManager oraz deklaracje metod i konstruktor�w

	@author Tomasz Kilja�czyk
	@version 02/11/18
*/

#ifndef SEXYOS_FILEMANAGER_H
#define SEXYOS_FILEMANAGER_H

#define _CRT_SECURE_NO_WARNINGS

#include <ctime>
#include <string>
#include <array>
#include <bitset>
#include <vector>
#include <unordered_map>

/*
	To-do:
	- przerobi� zapisywanie i odczytywanie z nazw na �cie�ki plik�w
	- doda� zabezpieczenie do zapisywania i odczytywania danych tylko dla plik�w otwartych
*/

//Klasa zarz�dcy przestrzeni� dyskow� i systemem plik�w
class FileManager {
private:
	using u_int = unsigned int;

	//--------------- Definicje sta�ych statycznych -------------
	static const size_t BLOCK_SIZE = 32;	   	    //Rozmiar bloku (bajty)
	static const size_t DISK_CAPACITY = 1024;       //Pojemno�� dysku (bajty)
	static const u_int BLOCK_INDEX_NUMBER = 3;	    //Warto�� oznaczaj�ca d�ugo�� pola blockDirect i blok�w niebezpo�rednich
	static const u_int INODE_NUMBER_LIMIT = 32;     //Maksymalna ilo�� element�w w katalogu
	static const u_int MAX_FILENAME_LENGTH = 7;        //Maksymalna d�ugo�� �cie�ki
	static const bool BLOCK_FREE = false;           //Warto�� oznaczaj�ca wolny blok
	static const bool BLOCK_OCCUPIED = !BLOCK_FREE; //Warto�� oznaczaj�ca zaj�ty blok
	/**Maksymalny rozmiar pliku obliczony na podstawie maksymalnej ilo�ci indeks�w*/
	static const size_t MAX_FILE_SIZE = (BLOCK_INDEX_NUMBER * 2) * BLOCK_SIZE;



	//---------------- Definicje struktur i klas ----------------

	//Klasa bloku indeksowego - przechowuje tablic� indeks�w.
	class IndexBlock {
	private:
		//Tablica indeks�w/blok�w indeksowych
		std::array<u_int, BLOCK_INDEX_NUMBER> value;
	public:
		IndexBlock() = default;
		virtual ~IndexBlock() = default;

		const u_int size() const { return value.size(); }
		void clear();

		u_int& operator [] (const size_t& index);
		const u_int& operator [] (const size_t& index) const;
	};

	//Klasa i-w�z�a - zawiera podstawowe informacje o pliku
	class Inode {
	public:
		//Podstawowe informacje
		size_t blocksOccupied = 0;   //Ilo�� zajmowanych blok�w
		size_t realSize = 0;         //Rzeczywisty ozmiar pliku
		std::array<u_int, BLOCK_INDEX_NUMBER> directBlocks; //Bezpo�rednie bloki
		IndexBlock singleIndirectBlocks; //Niebespo�redni blok indeksowy, zpisywany na dysku

		//Dodatkowe informacje
		tm creationTime = tm();	 //Czas i data utworzenia
		tm modificationTime = tm(); //Czas i data ostatniej modyfikacji pliku

		bool flagOpen = false; //Flaga otwarcia (true - plik otwarty, false - plik zamkni�ty)

		Inode();

		/**
			Konstruktor domy�lny.
		*/
		explicit Inode(std::string type_) : directBlocks() {}

		virtual ~Inode() = default;

		void clear();
	};


	//Prosta klasa dysku (imitacja fizycznego) - przechowuje dane + system plik�w
	class Disk {
	public:
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
			@param data Dane typu string.
			@return void.
		*/
		void write(const u_int& begin, const std::string& data);

		/**
			Odczytuje dane zadanego typu (je�li jest on zaimplementowany) w wskazanym przedziale.

			@param begin Indeks od kt�rego dane maj� by� odczytywane.
			@return zmienna zadanego typu.
		*/
		template<typename T>
		const T read(const u_int& begin) const;
	} DISK;
	struct FileSystem {
		u_int freeSpace{ DISK_CAPACITY }; //Zawiera informacje o ilo�ci wolnego miejsca na dysku (bajty)

		//Wektor bitowy blok�w (domy�lnie: 0 - wolny blok, 1 - zaj�ty blok)
		std::bitset<DISK_CAPACITY / BLOCK_SIZE> bitVector;

		/**
		 Tablica i-w�z��w
		 */
		std::array<Inode, INODE_NUMBER_LIMIT> inodeTable;
		//Pomocnicza tablica 'zaj�to�ci' i-w�z��w (1 - zaj�ty, 0 - wolny).
		std::bitset<INODE_NUMBER_LIMIT> inodeBitVector;
		std::unordered_map<std::string, u_int> rootDirectory;

		/**
			Konstruktor domy�lny. Wpisuje katalog g��wny do tablicy iW�z��w.
		*/
		FileSystem();

		const u_int get_free_inode_id();

		void reset();
	} FileSystem; //System plik�w FileSystem


	//------------------- Definicje zmiennych -------------------
	bool messages = false; //Zmienna do w��czania/wy��czania powiadomie�
	bool detailedMessages = false; //Zmienna do w��czania/wy��czania szczeg�owych powiadomie�
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
	bool file_create(const std::string& name);

	/**
		Zapisuje podane dane w danym pliku.

		@param name Nazwa pliku.
		@param data Dane do zapisu.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_write(const std::string& name, const std::string& data);

	/**
		Odczytuje dane z podanego pliku.

		@param name Nazwa pliku.
		@return Wczytane dane.
	*/
	const std::string file_read_all(const std::string& name);

	/**
		Usuwa plik o podanej nazwie znajduj�cy si� w obecnym katalogu.
		Plik jest wymazywany z tablicy i-w�z��w oraz wektora bitowego.

		@param name Nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_delete(const std::string& name);

	bool file_open(const std::string& name);

	bool file_close(const std::string& path);



	//--------------------- Dodatkowe metody --------------------

	/**
		Formatuje dysk. Zeruje wektor bitowy, usuwa wszystkie i-w�z�y,
		tworzy nowy katalog g��wny.

		@return void.
	*/
	bool disk_format();

	/**
		Tworzy plik o podanej nazwie w obecnym katalogu i zapisuje w nim podane dane.

		@param name Nazwa pliku.
		@param data Dane typu string.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_create(const std::string& name, const std::string& data);

	/**
		Zmienia nazw� katalogu (w obecnej �cie�ce) o podanej nazwie.

		@param name Obecna nazwa pliku.
		@param changeName Zmieniona nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_rename(const std::string& name, const std::string& changeName);

	/**
		Zmienia zmienn� odpowiadaj�c� za wy�wietlanie komunikat�w.
		false - komunikaty wy��czone.
		true - komunikaty w��czone.

		@param onOff Czy komunikaty maj� by� w��czone.
		@return void.
	*/
	void set_messages(const bool& onOff);

	/**
		Zmienia zmienn� odpowiadaj�c� za wy�wietlanie szczeg�owych komunikat�w.
		false - komunikaty wy��czone.
		true - komunikaty w��czone.

		@param onOff Czy komunikaty maj� by� w��czone.
		@return void.
	*/
	void set_detailed_messages(const bool& onOff);

	//------------------ Metody do wy�wietlania -----------------

	/**
		Wy�wietla parametry systemu plik�w.

		@return void.
	*/
	void display_file_system_params() const;

	/**
		Wy�wietla informacje o wybranym katalogu.

		@return void.
	*/
	void display_root_directory_info();

	/**
		Wy�wietla informacje o pliku.

		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool display_file_info(const std::string& name);

	/**
		Wy�wietla struktur� katalog�w.

		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	void display_root_directory();

	/**
		Wy�wietla zawarto�� dysku w formie binarnej.

		@return void.
	*/
	void display_disk_content_binary();

	/**
		Wy�wietla zawarto�� dysku jako znaki.
		'.' - puste pole.

		@return void.
	*/
	void display_disk_content_char();

	/**
		Wy�wietla wektor bitowy.

		@return void.
	*/
	void display_bit_vector();


private:
	//------------------- Metody Sprawdzaj�ce -------------------

	/**
		Sprawdza czy nazwa pliku jest u�ywana w katalogu g��wnym.

		@param name Nazwa pliku
		@return Prawda, je�li nazwa u�ywana, inaczej fa�sz.
	*/
	const bool check_if_name_used(const std::string& name);

	/**
		Sprawdza czy jest miejsce na dane o zadaniej wielko�ci.

		@param dataSize Rozmiar danych, dla kt�rych b�dziemy sprawdzac miejsce.
		@return void.
	*/
	const bool check_if_enough_space(const u_int& dataSize) const;



	//-------------------- Metody Obliczaj�ce -------------------

	/**
		Oblicza ile blok�w zajmie podany string.

		@param dataSize D�ugo�� danych, kt�rych rozmiar na dysku b�dzie obliczany.
		@return Ilo�� blok�w jak� zajmie string.
	*/
	const u_int calculate_needed_blocks(const size_t& dataSize) const;

	/**
		Zwraca rozmiar podanego katalogu.

		@return Rozmiar podanego katalogu.
	*/
	const size_t calculate_directory_size();

	/**
		Zwraca rzeczywisty rozmiar podanego katalogu.

		@return Rzeczywisty rozmiar podanego katalogu.
	*/
	const size_t calculate_directory_size_on_disk();



	//--------------------- Metody Alokacji ---------------------

	/**
		Zmniejsza lub zwi�ksza plik do podanego rozmiaru.

		@param file Wska�nik na plik, kt�rego rozmiar chcemy zmieni�.
		@param neededBlocks Ilo�� blok�w do alokacji.
		@return void.
	*/
	void file_truncate(Inode* file, const u_int& neededBlocks);

	/**
		Dodaje do pliku podane indeksy blok�w.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@param blocks Numery blok�w do alokacji.
		@return void.
	*/
	void file_add_indexes(Inode* file, const std::vector<u_int>& blocks) const;

	/**
		Przeprowadza dealokacje danych pliku, czyli usuwa z pliku indeksy blok�w
		oraz zmienia warto�ci w wektorze bitowym.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@return void.
	*/
	void file_deallocate(Inode* file);

	/**
		Alokuje przestrze� na podane bloki. Zmienia warto�ci w wektorze bitowym,
		aktualizuje warto�� zajmowanych blok�w przez plik oraz wywo�uje funkcj�
		file_add_indexes.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@param blocks Numery blok�w do alokacji.
		@return void.
	*/
	void file_allocate_blocks(Inode* file, const std::vector<u_int>& blocks);

	/**
		Obs�uguje proces zwi�kszania liczby zaalokowanych blok�w na dane pliku.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@param neededBlocks Liczba blok�w do alokacji.
		@return void.
	*/
	void file_allocation_increase(Inode* file, const u_int& neededBlocks);

	/**
		Obs�uguje proces zwi�kszania liczby zaalokowanych blok�w na dane pliku.

		@param file Wska�nik na plik, do kt�rego chcemy doda� indeksy.
		@param neededBlocks Liczba blok�w do alokacji.
		@return void.
	*/
	void file_allocation_decrease(Inode* file, const u_int& neededBlocks);

	/**
	Znajduje nieu�ywane bloki do zapisania pliku bez dopasowania do luk w blokach

	@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
	@return Zestaw indeks�w blok�w mo�liwych do zaalokowania.
	*/
	const std::vector<u_int> find_unallocated_blocks_fragmented(u_int blockNumber);

	/*
		Znajduje nieu�ywane bloki do zapisania pliku metod� best-fit.

		@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Zestaw indeks�w blok�w mo�liwych do zaalokowania.
	*/
	const std::vector<u_int> find_unallocated_blocks_best_fit(const u_int& blockNumber);

	/*
		Znajduje nieu�ywane bloki do zapisania pliku. Najpierw uruchamia funkcj�
		dzia�aj�c� metod� best-fit, je�li funkcja nie znajdzie dopasowania to
		uruchamiana jest funkcja znajduj�c� pierwsze jakiekolwiek wolne bloki i
		wprowadzaj�ca fragmentacj� danych.

		@param blockNumber Liczba blok�w na jak� szukamy miejsca do alokacji.
		@return Zestaw indeks�w blok�w mo�liwych do zaalokowania.
	*/
	const std::vector<u_int> find_unallocated_blocks(const u_int& blockNumber);



	//----------------------- Metody Inne -----------------------

	/**
		Zapisuje wektor fragment�w File.data na dysku.

		@param file Wska�nik na plik kt�rego dane b�d� zapisane na dysku.
		@param data Dane do zapisania na dysku.
		@return void.
	*/
	void file_write(Inode* file, const std::string& data);

	/**
		Wczytuje dane pliku z dysku.

		@param file Wska�nik na plik kt�rego dane maj� by� wczytane z dysku.
		@return Dane pliku w postaci string.
	*/
	const std::string file_read_all(Inode* file) const;

	/**
		Zwraca aktualny czas i dat�.

		@return Czas i data.
	*/
	static const tm get_current_time_and_date();

	/**
		Zmienia warto�� w wektorze bitowym i zarz�dza polem freeSpace
		w strukturze FileSystem.

		@param block Indeks bloku, kt�rego warto�� w wektorze bitowym b�dzie zmieniona.
		@param value Warto�� do przypisania do wskazanego bloku ( BLOCK_FREE lub BLOCK_OCCUPIED)
		@return void.
	*/
	void change_bit_vector_value(const u_int& block, const bool& value);

	/**
		Dzieli string na fragmenty o rozmiarze BLOCK_SIZE.

		@param data String do podzielenia na fragmenty.
		@return Wektor fragment�w string.
	*/
	const std::vector<std::string> data_to_data_fragments(const std::string& data) const;
};

static FileManager fileManager;

#endif //SEXYOS_FILEMANAGER_H

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
	- przerobi� zapisywanie i odczytywanie z �cie�ek na nazwy
	- doda� zabezpieczenie do zapisywania i odczytywania danych tylko dla plik�w otwartych
	- doda� semafory
	- doda� odczyt i zapis sekwencyjny
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
	static const u_int MAX_FILENAME_LENGTH = 7;     //Maksymalna d�ugo�� �cie�ki

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
		Disk();

		//-------------------------- Metody -------------------------
		void write(const u_int& begin, const std::string& data);

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

	const bool check_if_name_used(const std::string& name);

	const bool check_if_enough_space(const u_int& dataSize) const;



	//-------------------- Metody Obliczaj�ce -------------------

	const u_int calculate_needed_blocks(const size_t& dataSize) const;

	const size_t calculate_directory_size();

	const size_t calculate_directory_size_on_disk();



	//--------------------- Metody Alokacji ---------------------

	void file_truncate(Inode* file, const u_int& neededBlocks);

	void file_add_indexes(Inode* file, const std::vector<u_int>& blocks) const;

	void file_deallocate(Inode* file);

	void file_allocate_blocks(Inode* file, const std::vector<u_int>& blocks);

	void file_allocation_increase(Inode* file, const u_int& neededBlocks);

	void file_allocation_decrease(Inode* file, const u_int& neededBlocks);

	const std::vector<u_int> find_unallocated_blocks_fragmented(u_int blockNumber);

	const std::vector<u_int> find_unallocated_blocks_best_fit(const u_int& blockNumber);

	const std::vector<u_int> find_unallocated_blocks(const u_int& blockNumber);



	//----------------------- Metody Inne -----------------------

	void file_write(Inode* file, const std::string& data);

	const std::string file_read_all(Inode* file) const;

	static const tm get_current_time_and_date();

	void change_bit_vector_value(const u_int& block, const bool& value);

	const std::vector<std::string> data_to_data_fragments(const std::string& data) const;
};

static FileManager fileManager;

#endif //SEXYOS_FILEMANAGER_H

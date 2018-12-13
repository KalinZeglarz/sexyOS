/**
	SexyOS
	FileManager.h
	Przeznaczenie: Zawiera klas� FileManager oraz deklaracje metod i konstruktor�w

	@author Tomasz Kilja�czyk
	@version 13/12/18
*/

/*
 * Aby �atwiej nawigowa� po moim kodzie polecam z�o�y� wszystko
 * Skr�t: CTRL + M + A
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

class PCB;

/*
	TODO:
	- doda� semafory (x)
	- doda� odczyt i zapis sekwencyjny (x)
	- dorobi� stuff z �adowaniem do pliku wymiany (x)
	- i z zapisywaniem z pliku wymiany do dysku (x)
*/

#define OPEN_R_MODE  std::bitset<2>{ 0,1 }
#define OPEN_W_MODE  std::bitset<2>{ 1,0 }
#define OPEN_RW_MODE std::bitset<2>{ 1,1 }

//Klasa zarz�dcy przestrzeni� dyskow� i systemem plik�w
class FileManager {
private:
	//--------------------------- Aliasy ------------------------
	using u_int = unsigned int;
	using u_short_int = unsigned short int;
	using u_char = unsigned char;



	//--------------- Definicje sta�ych statycznych -------------

	static const u_char BLOCK_SIZE = 32;	   	   //Rozmiar bloku (bajty)
	static const u_short_int DISK_CAPACITY = 1024; //Pojemno�� dysku (bajty)
	static const u_char BLOCK_INDEX_NUMBER = 3;	   //Warto�� oznaczaj�ca d�ugo�� pola blockDirect i blok�w niebezpo�rednich
	static const u_char INODE_NUMBER_LIMIT = 32;   //Maksymalna ilo�� element�w w katalogu
	static const u_char MAX_FILENAME_LENGTH = 16;  //Maksymalna d�ugo�� �cie�ki

	static const bool BLOCK_FREE = false;           //Warto�� oznaczaj�ca wolny blok
	static const bool BLOCK_OCCUPIED = !BLOCK_FREE; //Warto�� oznaczaj�ca zaj�ty blok

	//Maksymalny rozmiar danych
	static const u_short_int MAX_DATA_SIZE = (BLOCK_INDEX_NUMBER + BLOCK_SIZE / 2)*BLOCK_SIZE;

	//Maksymalny rozmiar pliku (wliczony blok indeksowy)
	static const u_short_int MAX_FILE_SIZE = MAX_DATA_SIZE + BLOCK_SIZE;



	//---------------- Definicje struktur i klas ----------------

	//Klasa i-w�z�a - zawiera podstawowe informacje o pliku
	class Inode {
	public:
		//Podstawowe informacje
		u_char blocksOccupied = 0; //Ilo�� zajmowanych blok�w (dane)
		u_short_int realSize = 0;  //Rzeczywisty rozmiar pliku
		std::array<u_int, BLOCK_INDEX_NUMBER> directBlocks;         //Bezpo�rednie bloki
		u_int singleIndirectBlocks; //Niebespo�redni blok indeksowy, zpisywany na dysku

		//Dodatkowe informacje
		tm creationTime = tm();	 //Czas i data utworzenia
		tm modificationTime = tm(); //Czas i data ostatniej modyfikacji pliku

		bool flagOpen = false; //Flaga otwarcia (true - plik otwarty, false - plik zamkni�ty)

		Inode();

		virtual ~Inode() = default;

		void clear();
	};

	struct Disk {
		//Tablica reprezentuj�ca przestrze� dyskow� (jeden indeks - jeden bajt)
		std::array<u_char, DISK_CAPACITY> space;

		//----------------------- Konstruktor -----------------------
		Disk();

		//-------------------------- Metody -------------------------
		void write(const u_short_int& begin, const std::string& data);
		void write(const u_short_int& begin, const std::array<u_int, BLOCK_SIZE / 2>& data);

		const std::string read_str(const u_int& begin) const;
		const std::array<u_int, BLOCK_SIZE / 2> read_arr(const u_int& begin) const;
	} disk; //Struktura dysku
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
	} fileSystem; //System plik�w fileSystem

	class FileReader {
	private:
		std::string buffer;
		u_short_int posPointer = 0;
		Disk* disk;
		Inode* file;

	public:
		FileReader(Disk* disk, Inode* inode) : disk(disk), file(inode) {}

		void read(const u_short_int& byteNumber);

		void resetPosPointer() { posPointer = 0; }
	};



	//------------------- Definicje zmiennych -------------------
	bool messages = false; //Zmienna do w��czania/wy��czania powiadomie�
	bool detailedMessages = false; //Zmienna do w��czania/wy��czania szczeg�owych powiadomie�
	std::unordered_map<std::string, u_int> usedFiles;


public:
	//----------------------- Konstruktor -----------------------
	/**
		Konstruktor domy�lny. Przypisuje do obecnego katalogu katalog g��wny.
	*/
	explicit FileManager() = default;



	//-------------------- Podstawowe Metody --------------------
	/**
		Tworzy plik o podanej nazwie w obecnym katalogu. Po
		stworzeniu plik jest otwarty w trybie do zapisu.

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

	const bool file_read(const std::string& name, const u_short_int& memoryAddress, const u_short_int& byteNumber);

	/**
		Odczytuje wszystkie dane z podanego pliku.

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

	bool file_close(const std::string& name);



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
	static void display_file_system_params();

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

	void file_add_indexes(Inode* file, const std::vector<u_int>& blocks);

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

#endif //SEXYOS_FILEMANAGER_H

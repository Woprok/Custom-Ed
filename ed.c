//File ed.c
//Author: Miroslav Valach (Woprok)
//Disclaimer:
//Tento kod zodpoveda vasmu zadaniu najlepsie ako sa da. 
//Kedze je ale prakticky tak volne napisane ze si tam clovek moze vymysliet cokolvek a stale to bude splnat zadanie.
//Tak z velkou pravdepodobnostou neprejde automaticky testami.
//Ak ocakavate nieco co prejde automatickymi testami tak by ste mali omnoho lepsie specifikovat ake edge case to ma mat osetrene

//Before continuing fck parsing, fck gnu ed, this is tyranny, noone should be forced to write trash software like gnu ed
//I could find infinity amount of reason why is ed trash and you should never again give task to write anything that even resembles gnu ed
//Just why would anyone designing gnu ed make so many critical mistakes in design decisions ... 

//at some point this code went wrong direction or there was no other reasonable way how to write it, in the end its fck gnu ed
//program calls exit only in main yay//and in all cases when it goes ...

/*	_GNU_SOURCE
	This program should be compiled on UNIX systems, which should have extended C standard.
	In case this is not true provide implementation of:
		ssize_t 													//signed size_t
		ssize_t getline(char **lineptr, size_t *n, FILE *stream)	//read line from stream see _GNU_SOURCE documentation	
*/
#ifndef _GNU_SOURCE 
	#define _GNU_SOURCE 
#endif

//Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <limits.h>

/*	DEBUG prints
	In case you are interested to get more information on program run.
	Warning this may be little overkill.
*/
//#define DEBUG_PRINT fprintf
#ifndef DEBUG_PRINT
	#define DEBUG_PRINT fake_print
	void* fake_print(void* a, ...) { return a; }
#endif

//Optional features
#define OPTIONS_UNWANTED 1
#define FILE_IS_MANDATORY 0
#define	ONLY_SIMPLE_FILE_NAMES 0
#define PRINT_ERROR_ONCE 0

//CONSTANTS
#define nullptr NULL
#define STRING_EMPTY "\0"
#define LINE_LENGTH 1024
#define FILE_NAME_LENGTH 1024
#define PROMPT_LENGTH 1024
#define ARG_LENGTH 1024
#define MESSAGE_COUNT 10
#define MESSAGE_LENGTH 256

/*	Double linked list implementation
 	Functions:
 		void list_destroy(head*)
 		bool list_get_node(head*, const int, node**, int*)
 		bool list_push_at(head*, const int, const char*)
 		void list_pop_at(head*, const int)
 		void list_print_to_stream(head*, FILE* const int, const int, bool)
 	Overview:
 		head is holder of whole list, all operations on list requires head
 		count represents amount of items in the list
*/
typedef struct node
{
	char content[LINE_LENGTH];
	struct node* predeccessor;
	struct node* successor; 
} node;

const node NODE_DEFAULT = { STRING_EMPTY, nullptr, nullptr };

typedef struct head
{
	int count;
	struct node* successor; 
} head;

const head HEAD_DEFAULT = { 0, nullptr };

//Free all allocated resources and set successor_ptr to nullptr 
void list_destroy(head* _head)
{	
DEBUG_PRINT(stderr, "%s\n", "list_destroy::called");
	node* current_node = _head->successor;
	node* next_node = nullptr;
	while (current_node != nullptr)
	{
		next_node = current_node->successor;
		free(current_node);
		current_node = next_node;
	}	
	_head->successor = nullptr;
DEBUG_PRINT(stderr, "%s\n", "list_destroy::finished");
}

bool list_get_node(head* _head, const int _index, node** _out_node, int* _out_index)
{
DEBUG_PRINT(stderr, "%s\n", "list_get_node::called");
	if (_head->count == 0 || _index <= 0 || _index > _head->count)
	{
DEBUG_PRINT(stderr, "%s: in:%d\n", "list_get_node::not_found", _index);
		return false;
	}
	(*_out_node) = _head->successor;
	(*_out_index) = 1;
	while ((*_out_index) != _index)
	{

		(*_out_node) = (*_out_node)->successor;
		++(*_out_index);
	}
DEBUG_PRINT(stderr, "%s in:%d out::%d\n", "list_get_node::found", _index, (*_out_index));
	return true;
}

bool list_push_at(head* _head, const int _index, const char* _content)
{
DEBUG_PRINT(stderr, "%s\n", "list_push_at::called");
	node* new_node = (node*)malloc(sizeof(node));
	if (new_node == nullptr)
	{
DEBUG_PRINT(stderr, "%s\n", "list_push_at::free_space_failed");
		fprintf(stderr, "%s\n", "Out of memory.");
		fflush(stderr);
		return false;
	}
	strcpy(new_node->content, _content);
DEBUG_PRINT(stderr, "%s\n", "list_push_at::free_space_passed");
	if (_index == 0)
	{
DEBUG_PRINT(stderr, "%s\n", "list_push_at::index==0::started");
		new_node->predeccessor = nullptr;
		if (_head->count == 0) //first item on head
		{
DEBUG_PRINT(stderr, "%s\n", "list_push_at::index==0::first");
			new_node->successor = nullptr;
		}
		else 
		{
DEBUG_PRINT(stderr, "%s\n", "list_push_at::index==0::not_first");
			_head->successor->predeccessor = new_node;
			new_node->successor = _head->successor;
		}
		_head->successor = new_node;	
		++(_head->count);
DEBUG_PRINT(stderr, "%s\n", "list_push_at::index==0::ended");
		return true;
	}

DEBUG_PRINT(stderr, "%s\n", "list_push_at::index!=0::started");
	node* current_node = nullptr;
	int current_index = 1;
	if (!list_get_node(_head,_index, &current_node, &current_index))
	{
DEBUG_PRINT(stderr, "%s\n", "list_push_at::skipped");
		free(new_node);
		return false;
	}
	node* successor_node = current_node->successor;
	++(_head->count);
	new_node->predeccessor = current_node;
	current_node->successor = new_node;
	if (successor_node == nullptr) //we are changing tail item
	{
		new_node->successor = nullptr;
DEBUG_PRINT(stderr, "%s\n", "list_push_at::index!=0::ended_tail");
		return true;
	}
	else //we are changing middle
	{
		new_node->successor = successor_node;
		successor_node->predeccessor = new_node;
	}
DEBUG_PRINT(stderr, "%s\n", "list_push_at::index!=0::ended_middle");
	return true;
}

//Removes node at index
void list_pop_at(head* _head, const int _index)
{
DEBUG_PRINT(stderr, "%s\n", "list_pop_at::called");
	node* current_node = nullptr;
	int current_index = 1;
	if (!list_get_node(_head, _index, &current_node, &current_index))
	{
DEBUG_PRINT(stderr, "%s\n", "list_pop_at::skipped");
		return;
	}

	node* predeccessor_node = current_node->predeccessor;
	node* successor_node = current_node->successor;
	
	if (predeccessor_node != nullptr)
	{
DEBUG_PRINT(stderr, "%s\n", "list_pop_at::removing::is_not_head");
		predeccessor_node->successor = successor_node;	
	}
	else
	{
DEBUG_PRINT(stderr, "%s\n", "list_pop_at::removing::is_head");
		_head->successor = (successor_node != nullptr) ? successor_node : nullptr;		
	}
	if (successor_node != nullptr)
	{
DEBUG_PRINT(stderr, "%s\n", "list_pop_at::removing::is_not_tail");
		successor_node->predeccessor = predeccessor_node;	
	}
	else
	{
DEBUG_PRINT(stderr, "%s\n", "list_pop_at::removing::is_tail");		
	}
	
	free(current_node);	
	_head->count--;
DEBUG_PRINT(stderr, "%s\n", "list_pop_at::finished");
}

//Prints list content from index up to count, optional to print line index
ssize_t list_print_to_stream(head* _head, FILE* _stream, const int _index, const int _count, bool _print_index)
{
DEBUG_PRINT(stderr, "%s\n", "list_print_to_stream::called");
	node* current_node = nullptr;
	int current_index = 1;
	int printed = 0;	
	ssize_t write_count = 0;
	if (!list_get_node(_head, _index, &current_node, &current_index))
	{
DEBUG_PRINT(stderr, "%s\n", "list_print_to_stream::skipped");		
		return write_count;
	}
DEBUG_PRINT(stderr, "%s\n", "list_print_to_stream::printing");
	while(current_node != nullptr && printed < _count)
	{
DEBUG_PRINT(stderr, "%s%d\n", "list_print_to_stream::printed: ", current_index);
		if (_print_index)
			write_count += fprintf(_stream, "%d%c%s", current_index, '\t', current_node->content);
		else
			write_count += fprintf(_stream, "%s", current_node->content);
		
		fflush(_stream);
		current_node = current_node->successor;
		++printed;
		++current_index;
	}
DEBUG_PRINT(stderr, "%s\n", "list_print_to_stream::finished");
	return write_count;
}




typedef struct file_holder
{
	bool is_open;
	int error_code;
	char file_name[FILE_NAME_LENGTH];
	FILE* file;
} file_holder;

const file_holder FILE_HOLDER_DEFAULT = { false, 0, STRING_EMPTY, nullptr }; 

typedef struct option_holder
{
	bool is_set;
	bool help;
	bool version;
	bool traditional;
	bool loose_exit_status;
	bool restricted;
	bool silent;
	bool verbose;
	bool prompt;
	char prompt_string[PROMPT_LENGTH];
} option_holder;

const option_holder OPTION_HOLDER_DEFAULT = { false, false, false, false, false, false, false, false, false, STRING_EMPTY };

typedef struct address
{
	bool is_set;
	bool is_num;
	int numeric_value;
	char special_value; //VALID '.', '?' //INVALID ',' + ALL OTHER
} address;

const address ADDRESS_DEFAULT = { false, false, 0, ',' }; //DEFAULT MUST CONTAIN INVALID CHARACTER

typedef struct command_address
{
	bool no_address_defined; //false no address was supplied
	int from_address; //0 not defined
	int address_range; //0 not defined
} command_address;

const command_address COMMAND_ADDRESS_DEFAULT = { true, 0, 0 };

typedef struct address_holder
{
	int address_value;
	bool modified_buffer;
	bool pre_quit;
} address_holder;

const address_holder ADDRESS_HOLDER_DEFAULT = { 1, false, false };

typedef struct error_holder
{
	bool is_set;
	bool auto_print;
	bool printed; //print all messages once and never again until next error happens
	int code;
	char messages[MESSAGE_COUNT][MESSAGE_LENGTH];
} error_holder;

const error_holder ERROR_HOLDER_DEFAULT = { false, false, true, -1, { "Unknown command\n\0", "Invalid command suffix\n\0", "Invalid address\n\0", 
"Unexpected address\n\0", "No current filename\n\0", "Wrong filename\n\0", "command \"i\"  input exception\n\0", "Unexpected command suffix\n\0", 
"Cannot open input file\n\0", "Warning: buffer modified\n\0" } };

typedef struct command
{
	bool requires_address;
	bool requires_arg;
	char identifier;
	char arg[ARG_LENGTH];
} command;

const command COMMAND_DEFAULT = { false, false, '0', "\0" };

void update_error_check_print(error_holder* _error, bool _run)
{
#if PRINT_ERROR_ONCE
	if (!_error->printed)
#endif
		if ((_error->auto_print || _run) && _error->code >= 0 && _error->code < MESSAGE_COUNT)
		{
			fprintf(stderr, "%s", _error->messages[_error->code]);
			fflush(stderr);
			_error->printed = true;
		}
}

void update_error_holder_code_only(error_holder* _error, int _code)
{
	if (_code >= 0 && _code < MESSAGE_COUNT)
	{
		_error->is_set = true;
		_error->code = _code;
		_error->printed = false;
	}
}

void update_error_holder_code(error_holder* _error, int _code, bool _run)
{
	if (_code >= 0 && _code < MESSAGE_COUNT)
	{
		fprintf(stderr, "%s\n", "?");
		fflush(stderr);
		_error->is_set = true;
		_error->code = _code;
		_error->printed = false;
	}
	update_error_check_print(_error, _run);
}

void update_error_holder(error_holder* _error, bool _run, bool _swap_toggle)
{
	if (_swap_toggle)
	{
		_error->auto_print = !_error->auto_print;
	}
	update_error_check_print(_error, _run);
}

//true if file was opened, false if file was already opened or file can't be opened, updates error_code.
bool file_open(file_holder* _file, error_holder* _error)
{
DEBUG_PRINT(stderr, "%s\n", "file_open::started");
	if (_file->is_open)
	{
DEBUG_PRINT(stderr, "%s\n", "file_open::already_opened");
		return false;
	}	
	if ((_file->file = fopen(_file->file_name, "r")) == nullptr) 
    {
		_file->error_code = errno;
		errno = 0;
		fprintf(stderr, "%s: %s\n", _file->file_name, strerror(_file->error_code));
		fflush(stderr);
		update_error_holder_code_only(_error, 8);
DEBUG_PRINT(stderr, "%s\n", "file_open::failed");
        return false;
    }
    _file->is_open = true;
DEBUG_PRINT(stderr, "%s\n", "file_open::success");
    return true;
}

//true if file was closed, false if file was already closed
bool file_close(file_holder* _file)
{
DEBUG_PRINT(stderr, "%s\n", "file_close::started");
	if (_file->is_open)
	{
		fclose(_file->file);	
		_file->is_open = false;
DEBUG_PRINT(stderr, "%s\n", "file_close::success");
		return true;
	}
DEBUG_PRINT(stderr, "%s\n", "file_close::already_closed");
	return false;
}

bool parse_option_monogram(option_holder* _option, const char* _monogram) 
{
	int i = 1;
	while(_monogram[i] != '\0')
	{
		switch(_monogram[i])
		{
			case 'h':
				if (_option->help)
					return false;
				_option->help = true;
			 	break;
			case 'V':
				if (_option->version)
					return false;
				_option->version = true;
				break;
			case 'G':
				if (_option->traditional)
					return false;
				_option->traditional = true;
				break;
			case 'l':
				if (_option->loose_exit_status)
					return false;
				_option->loose_exit_status = true;
				break;
			case 'r':
				if (_option->restricted)
					return false;
				_option->restricted = true;
				break;
			case 's':
				if (_option->silent)
					return false;
				_option->silent = true;
				break;
			case 'v':
				if (_option->verbose)
					return false;
				_option->verbose = true;
				break;
			case 'p': //can be only as last option				
				if (_option->prompt)
					return false;
				_option->prompt = true;

				if (strstr(_monogram, "p=") == nullptr || strstr(_monogram, "p=") != _monogram + i)
				{
					fprintf(stderr, "ed: illegal option -- %s\n", _monogram + i);
					fflush(stderr);
					fprintf(stderr, "usage: ed file\n");
					fflush(stderr);
					return false;
				}
				strcpy(_option->prompt_string, _monogram + i + strlen("p="));
				return true;
				break;
			default: //error	
				fprintf(stderr, "ed: illegal option -- %c\n", _monogram[i]);
				fflush(stderr);
				fprintf(stderr, "usage: ed file\n");	
				fflush(stderr);
				return false;
				break;
		}
		++i;
	}
	return true;
}

bool parse_option_expanded(option_holder* _option, const char* _expanded) 
{ 
	if (strcmp(_expanded, "--help") == 0)
	{
		if (_option->help)
			return false;
		_option->help = true;
		return true;
	}
	if (strcmp(_expanded, "--version") == 0)
	{				
		if (_option->version)
			return false;
		_option->version = true;
		return true;
	}
	if (strcmp(_expanded, "--traditional") == 0)
	{				
		if (_option->traditional)
			return false;
		_option->traditional = true;
		return true;
	}
	if (strcmp(_expanded, "--loose_exit_status") == 0)
	{
		if (_option->loose_exit_status)
			return false;
		_option->loose_exit_status = true;
		return true;
	}
	if (strcmp(_expanded, "--restricted") == 0)
	{
		if (_option->restricted)
			return false;
		_option->restricted = true;
		return true;
	}
	if (strcmp(_expanded, "--quiet" ) == 0 || strcmp(_expanded, "--silent") == 0)
	{
		if (_option->silent)
			return false;
		_option->silent = true;
		return true;
	}
	if (strcmp(_expanded, "--verbose") == 0)
	{
		if (_option->verbose)
			return false;
		_option->verbose = true;
		return true;
	}
	if (strstr(_expanded, "--prompt=") != nullptr && strstr(_expanded, "--prompt=") == _expanded) 
	{				
		if (_option->prompt)
			return false;
		_option->prompt = true;
		strcpy(_option->prompt_string, _expanded + strlen("--prompt="));
		return true;
	}
	fprintf(stderr, "ed: illegal option -- %s\n", _expanded + strlen("--"));
	fflush(stderr);
	fprintf(stderr, "usage: ed file\n");
	fflush(stderr);	
	return false;
}

void file_set(file_holder* _file, const char* _file_name)
{
	strcpy(_file->file_name, _file_name);
}

bool parse_args(const int _argc, const char** _argv, option_holder* _option, file_holder* _file)
{
	for (int i = 1; i < _argc; ++i)
	{
DEBUG_PRINT(stderr, "argv[%d] = \"%s\"\n", i, _argv[i]);
		if (_argv[i][0] == '\0') //i am nice person that ignores empty args
		{
			continue;
		}
		if (strlen(_argv[i]) >= ARG_LENGTH) //there needs to be some kind of a limit... again not specified
		{
			fprintf(stderr, "ed: illegal argument -- %s\n", _argv[i]); //this is just result of unspecified behaviour
			fflush(stderr);
			fprintf(stderr, "usage: ed file\n");
			fflush(stderr);
			return false;
		}
		if (_argv[i][0] == '-' && _argv[i][1] == '-' && _argv[i][2] != '\0')
		{
			if (parse_option_expanded(_option, _argv[i]))
			{
				_option->is_set = true;
				continue;
			}
			return false;
		}
		if (_argv[i][0] == '-' && _argv[i][1] != '\0')
		{
			if (parse_option_monogram(_option, _argv[i]))
			{
				_option->is_set = true;
				continue;
			}
			return false;
		}
		if (_argv[i][0] != '-' && (_file->file_name)[0] == '\0') //limit is one file
		{
			file_set(_file, _argv[i]);
			continue;
		}

		fprintf(stderr, "ed: illegal argument -- %s\n", _argv[i]); //this is just result of unspecified events
		fflush(stderr);
		fprintf(stderr, "usage: ed file\n");
		fflush(stderr);
		return false;
	}
	return true;
}

bool load_file(file_holder* _file, head* _head)
{
DEBUG_PRINT(stderr, "load_file::started\n");
	char *new_line = nullptr;
	size_t size = 0;
	ssize_t line_length = 0;
	ssize_t total_length = 0;
	while ((line_length = getline(&new_line, &size, _file->file)) != -1) 
	{
DEBUG_PRINT(stderr, "%d: %d: %d\n", size, line_length, total_length);
	total_length += line_length;
DEBUG_PRINT(stderr, "load_file::line_length: %ld\n", line_length);
		if (line_length > LINE_LENGTH)
		{
			fprintf(stderr, "Line length exceeded: %d, read value: %ld\n", LINE_LENGTH, line_length);
			fflush(stderr);
DEBUG_PRINT(stderr, "load_file::error::line_length: %ld\n", line_length);
			free(new_line);
			return false;			
		}
DEBUG_PRINT(stderr, "load_file::read_line: %s\n", new_line);
		if (!list_push_at(_head, _head->count, new_line)) //run out of memory
		{
			free(new_line);
			return false;
		}
	}
DEBUG_PRINT(stderr, "%d: %d: %d\n", size, line_length, total_length);
	fprintf(stderr, "%lu\n", total_length);

	free(new_line);
	if (errno != 0)
	{ 
		fprintf(stderr, "%s\n", strerror(errno));
		fflush(stderr);
DEBUG_PRINT(stderr, "load_file::error::errno: %d\n", errno);
		errno = 0;
		return false;
	}
DEBUG_PRINT(stderr, "load_file::success:\n");
	return true;
}

//validates address_field must be substring with '\0', on valid updates address_pass
bool convert_address_field(address* _address, const bool _special_character, const char* _address_field)
{
	_address->is_set = true;
	if (_special_character)
	{
		_address->is_num = false;
		_address->special_value = _address_field[0]; //must be first char in string
	}
	else
	{
		char* end;
		long result = strtol(_address_field, &end, 10);
		if (result > INT_MAX || result < INT_MIN) //incorrect address for converting, return in case of error is long min or max
			return false;

		_address->is_num = true;
		_address->numeric_value = (int) result;
	}
	return true;
}

//parse until start of command or end of string
//command starts at beginning _command_offset = 0
//command contains bad address _command_offset != 0
bool convert_address(address* _address, int* _command_offset, const char* _field)
{
DEBUG_PRINT(stderr, "%s\n", "convert_address::called");
	bool no_error = true;
	bool is_special = false;
	int character_count = 0;
	int i = 0;
	
	if (_field[i] == '.' || _field[i] == '$') //parse special
	{
DEBUG_PRINT(stderr, "%s\n", "convert_address::special_value");
		i = 1; //this makes i equal to first command char
		is_special = true;
		character_count = i;
	}
	else if (isdigit(_field[i]) || _field[i] == '-') //parse number
	{
DEBUG_PRINT(stderr, "%s\n", "convert_address::numeric_value");
		if (_field[i] == '-') //check sign
			++i;
		while(isdigit(_field[i]) && _field[i] != '\0')
			++i; //this makes i equal to first command char
		character_count = i;
	}
	else //no address or incorrect address format from the start
	{
DEBUG_PRINT(stderr, "%s\n", "convert_address::error");
		(*_command_offset) = 0; //command starts at beginning
		return false;
	}

	char* buffer = malloc(sizeof(char) * (character_count + 1-1)); //+ '\0'
	strncpy(buffer, _field, character_count); //get substring copying valid chars
	buffer[character_count + 1-1] = '\0'; //terminate string
DEBUG_PRINT(stderr, "buffer: %d\n", character_count);
DEBUG_PRINT(stderr, "add: %s\n", buffer);

	no_error = convert_address_field(_address, is_special, buffer);

	(*_command_offset) = i; //return first character of command
	free(buffer);
	return no_error;
}

//it was not specified what is min/maximum possible address entered so this assumes its integer
//fck gnu ed, its trash, why would you not force user to use delimeters...
bool obtain_address(address* _address_n, address* _address_m, int* _command_offset, const char* _line) 
{
DEBUG_PRINT(stderr, "%s\n", "obtain_address::called");
	_address_n->is_set = false;
	_address_m->is_set = false;
	int i = 0;
	while(_line[i] != ',' && _line[i] != '\0') //reach separator or eos in this reality we dont care about '\n' 
		++i; 
	
	if (_line[i] != ',') //single address
	{
DEBUG_PRINT(stderr, "%s\n", "obtain_address::convert_simple");
		return convert_address(_address_n, _command_offset, _line); //life is simple here
	}
	
	if (_line[i] == ',') //double address
	{
DEBUG_PRINT(stderr, "%s\n", "obtain_address::convert_double");
		int character_count_n = i;
		char* buffer_n = malloc(sizeof(char) * character_count_n + 1-1); //+ '\0'
		strncpy(buffer_n, _line, character_count_n); //get substring before delimeter
		buffer_n[character_count_n + 1-1] = '\0'; //terminate string

DEBUG_PRINT(stderr, "obtain_address::FIRST::%s\n", buffer_n);
		bool convert_n = convert_address(_address_n, _command_offset, buffer_n); // reality is sad and we need substring with '\0'

		free(buffer_n);

		++i; //move past delimeter
DEBUG_PRINT(stderr, "obtain_address::SECOND::%s\n", _line + i);
		bool convert_m = convert_address(_address_m, _command_offset, _line + i); //same as single address, this time with moved ptr

		//in case of error _command_offset is not null as there is at bare minimum a separator and single char behind begining
		//i is for skipped characters on call of convert
		(*_command_offset) = (*_command_offset) + i;

DEBUG_PRINT(stderr, "obtain_address::offset::%d\n", (*_command_offset));
		return convert_n == true && convert_m == true;
	}

DEBUG_PRINT(stderr, "%s\n", "obtain_address::unreachable_reached");
	//this is unreachable, but i am keeping ifs for clarity of code
	return false;
}

//this returns another object that just holds address, but in more usefull format
command_address* determine_address_validity(address* _address_n, address* _address_m, const int _command_offset, const bool _result, const int _current_line, const int _head_count)
{
DEBUG_PRINT(stderr, "%d %s\n", _command_offset, _result ? "true" : "false");
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::called");
	if (_result == false && _command_offset == 0)
	{
		command_address* empty_address = malloc(sizeof(command_address));
		empty_address->no_address_defined = true;
		empty_address->from_address = 0;
		empty_address->address_range = 0;
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::empty");
		return empty_address; //valid address based on address only, e.g. no address
	}
	if (_result == false && _command_offset != 0)
	{
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::nullptr");
		return nullptr; //this mean that there was at least start of address and then something bad happened or appeared
	}
	int n_value = 0;
	int m_value = 0;
	if (!_address_n->is_num) //we can be naive as previous checks should never let us get here
		n_value = (_address_n->special_value == '.') ? _current_line : _head_count - 1; 
	else 
		n_value = _address_n->numeric_value;

	if (_address_m->is_set)
	{
		if (!_address_m->is_num)
			m_value = (_address_m->special_value == '.') ? _current_line : _head_count - 1; 
		else 
			m_value = _address_m->numeric_value;
	}
	//nice now we have only numbers
	
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::setting");
	command_address* new_address = malloc(sizeof(command_address));
	new_address->no_address_defined = false;

	if (!_address_m->is_set)
	{
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::single");
		new_address->address_range = 0;
		if (n_value < 0) //substract address
		{
			if ((_current_line + n_value) < 1)
				return nullptr; //this means that this would reach address 0 or smaller
			else 
			{
				new_address->from_address = (_current_line + n_value);
				return new_address;
			}
		}
		if (n_value == 0 || n_value > _head_count)
			return nullptr; //this means its out of bounds
		//move address in all other cases	
		new_address->from_address = n_value; 
		return new_address;
	}

	if (_address_m->is_set)
	{
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double");
DEBUG_PRINT(stderr, "%s: %d <-> %d\n", "determine_address_validity::double", n_value, m_value);
		//1 10
		//10 1
		//-1 -10
		//-10 -1
		if (n_value > m_value)
			return nullptr; //this mean that n is smaller than m
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double::pass::1");
		if (n_value < 0 && (_current_line + n_value) < 1)
			return nullptr;
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double::pass::2");
		if (m_value < 0 && (_current_line + m_value) < 1)
			return nullptr;
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double::pass::3");
		if (n_value == 0 || n_value > _head_count || m_value == 0 || m_value > _head_count)
			return nullptr; //this mean that its out of bounds
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double::pass::4");
		if (n_value != m_value)
		{
			if (n_value > 0 && m_value > 0)
			{
				if ((m_value - n_value + 1) > _head_count)
					return nullptr;
			}
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double::pass::4.1");
			if (n_value < 0 && m_value < 0)
			{				
				if (((_current_line + m_value) - (_current_line + n_value) + 1) > _head_count)
					return nullptr;
			}
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double::pass::4.1");
		}			
		if (n_value > 0 && m_value > 0)
		{
			new_address->from_address = n_value;
			new_address->address_range = m_value;
			if (n_value != m_value)
				new_address->address_range = new_address->address_range - new_address->from_address + 1;
		}		
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double::pass::5");
		if (n_value < 0 && m_value < 0)
		{
			new_address->from_address = (_current_line + n_value);
			new_address->address_range = (_current_line + m_value);
			if (n_value != m_value)
				new_address->address_range = new_address->address_range - new_address->from_address + 1;
		}
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::double::pass::6");
		if ((n_value < 0 && m_value > 0) || (n_value > 0 && m_value < 0)) //just fcking decide what you want and dont combine both
		{
DEBUG_PRINT(stderr, "%s\n", "address::either both positive or both negative, we dont support combination, its already too much parsing code!");
			return nullptr;
		}		
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::return");
		return new_address;
	}
DEBUG_PRINT(stderr, "%s\n", "determine_address_validity::error");
	return nullptr;
}

command* get_command(error_holder* _error, const char* _command_line)
{
DEBUG_PRINT(stderr, "%s\n", _command_line);
	command* _com = malloc(sizeof(command));
	_com->requires_address = false;
	_com->requires_arg = false;
DEBUG_PRINT(stderr, "***COMMAND IS: %c\n", _command_line[0]);
	switch(_command_line[0])
	{
		case '\n':
DEBUG_PRINT(stderr, "%s\n", "get_command::command /n");
			_com->identifier = '\n';
		 	_com->requires_address = true;
			break;
		case 'p': 
			_com->identifier = 'p';
		 	_com->requires_address = true;
			break;
		case 'n': 
			_com->identifier = 'n';
		 	_com->requires_address = true;
			break;
		case 'H': 
			_com->identifier = 'H';
			break;
		case 'h': 
			_com->identifier = 'h';
			break;
		case 'q': 
			_com->identifier = 'q';
			break;
		case 'i': 
			_com->identifier = 'i';
		 	_com->requires_address = true;
			break;
		case 'd': 
			_com->identifier = 'd';
		 	_com->requires_address = true;
			break;
		case 'w': 
			_com->identifier = 'w';
		 	_com->requires_arg = true;
			break;
		default:
DEBUG_PRINT(stderr, "%s\n", "get_command::command error");
			update_error_holder_code(_error, 0, false);
			free(_com);
			return nullptr;
			break;
	}

DEBUG_PRINT(stderr, "%s\n", "get_command::parsed_ok");
	if (_com->requires_arg)
	{
DEBUG_PRINT(stderr, "%s\n", "get_command::expect space");
		if (_command_line[1] == ' ')
		{
DEBUG_PRINT(stderr, "%s\n", "get_command::found space");
			bool is_correct = true;
			const char* moved = _command_line + 2;
			int i = 0;
			while (moved[i] != '\0' && moved[i] != '\n')
			{
#if ONLY_SIMPLE_FILE_NAMES //popravde overit co je to suber je prilis vela, takze budeme uvazovat jednoduchu variantu iba alebo ziadnu
				if (isalnum(moved[i]) == false && moved[i] != '.') 
				{
DEBUG_PRINT(stderr, "%s\n", "get_command::doing_worst_possible_filename_check");
					is_correct = false;
					break;
				}
#endif
				++i;			
			}
			if (!is_correct)
			{ 
DEBUG_PRINT(stderr, "%s\n", "get_command::decided not file");
				update_error_holder_code(_error, 5, false);
				free(_com);
				return nullptr;
			}

			char* buffer = malloc(sizeof(char) * (i + 1-1)); //+ '\0'
			strncpy(buffer, moved, i); //get substring copying valid chars
			buffer[i + 1-1] = '\0'; //terminate string
			strcpy(_com->arg, buffer); //copy file_name;
			free(buffer);
DEBUG_PRINT(stderr, "get_command::FILE_ARG::%s\n", _com->arg);
			return _com;
		}
		else if (_command_line[1] == '\n' || _command_line[1] == '\0')
		{
			strcpy(_com->arg, "");
			return _com;
			//no action
		}
		else
		{
			update_error_holder_code(_error, 7, false);
			return nullptr;
		}
	}
	else if (!_com->requires_arg)
	{
		if (_command_line[1] == '\0' || _command_line[1] == '\n') //one letter command
		{
			return _com;
		}
		update_error_holder_code(_error, 1, false);
		return nullptr;
	}

DEBUG_PRINT(stderr, "%s\n", "get_command::unexpected_failure");
	free(_com);
	return nullptr;
}

void delete_command(head* _head, command_address* _address, address_holder* _current_address)
{
	_current_address->address_value = _address->from_address;
	int delete = _address->address_range;
	
	if (delete == 0)
	{
		list_pop_at(_head, _current_address->address_value);		
	}
	else
	{
		while (delete != 0)
		{
			list_pop_at(_head, _current_address->address_value);
			--delete;
		}		
	}
	_current_address->modified_buffer = true;

	if (_head->count == 0)
		_current_address->address_value = 0; //wtf? ed ? seriously ? why ?
	else
		_current_address->address_value = _current_address->address_value;
}

//gnu ed can accept 0 this ed will not accept it ever!
//this ed also does not know how to insert after last line
void insert_command(head* _head, command_address* _address, address_holder* _current_address, error_holder* _error) 
{
	char *new_line = nullptr;
	size_t size = 0;
	ssize_t line_length;
	
	int hacked = _address->from_address - 1;
	int inserted = 0;
	
	while ((line_length = getline(&new_line, &size, stdin)) != -1)
	{
DEBUG_PRINT(stderr, "%s", new_line);
		if (strcmp(new_line, ".\n") == 0)
		{
			break;
		}
		if (line_length > LINE_LENGTH) //we still care about line length
		{
DEBUG_PRINT(stderr, "Line length exceeded: %d, read value: %ld\n", LINE_LENGTH, line_length);
			update_error_holder_code(_error, 6, false);
			return;			
		}
		list_push_at(_head, hacked + inserted, new_line);
		++inserted;
	}
	_current_address->address_value = hacked + inserted;
	
	free(new_line);
	
	if (errno != 0)
	{ 
DEBUG_PRINT(stderr, "%s\n", strerror(errno));
		errno = 0;
		update_error_holder_code(_error, 6, false);
	}
}

void execute_address_command(head* _head, command* _command, error_holder* _error, address_holder* _current_address, command_address* _address)
{
if (_address->address_range == 0 && (_command->identifier == '\n' || _command->identifier == 'n' || _command->identifier == 'p'))
	 _address->address_range = 1;
DEBUG_PRINT(stderr, "Executing complicated: %c\n", _command->identifier);
	switch(_command->identifier)
	{
		case '\n':
DEBUG_PRINT(stderr, "Executing /n: %d f%d r%d\n", _current_address->address_value, _address->from_address, _address->address_range);
			list_print_to_stream(_head, stdout, _address->from_address, _address->address_range, false);
			_current_address->address_value = _address->from_address + _address->address_range -1;
			break;
		case 'p': 
			list_print_to_stream(_head, stdout, _address->from_address, _address->address_range, false);
			_current_address->address_value = _address->from_address + _address->address_range -1;
			break;
		case 'n': 
			list_print_to_stream(_head, stdout, _address->from_address, _address->address_range, true);
			_current_address->address_value = _address->from_address + _address->address_range -1;
			break;
		case 'i': 
			insert_command(_head, _address, _current_address, _error);			
			break;
		case 'd': 
			delete_command(_head, _address, _current_address);
			break;
	}
	_current_address->pre_quit = false;
}


void write_command(head* _head, error_holder* _error, command* _command, address_holder* _current_address)
{
DEBUG_PRINT(stderr, "%s\n", "command w file::called");
	FILE* file = nullptr;
	if ((file = fopen(_command->arg, "w")) == nullptr)
	{
DEBUG_PRINT(stderr, "%s\n", "command w file::already_closed");
DEBUG_PRINT(stderr, "%s\n", strerror(errno));
		errno = 0;
		update_error_holder_code(_error, 7, false);

		return;
	}	
	
DEBUG_PRINT(stderr, "%s %p\n", "command w file::printing", (void*)file);
	ssize_t write_count = list_print_to_stream(_head, file, 1, _head->count, false);
	fflush(file);
	fprintf(stdout, "%lu\n", write_count);
	fflush(stdout);

	_current_address->modified_buffer = false;

	if (errno != 0)
	{ 
DEBUG_PRINT(stderr, "%s\n", strerror(errno));
		update_error_holder_code(_error, 7, false);
		errno = 0;
	}
	fclose(file);
DEBUG_PRINT(stderr, "%s\n", "command w file::finished");
}

void execute_single_command(head* _head, command* _command, error_holder* _error, address_holder* _current_address)
{
DEBUG_PRINT(stderr, "Executing simple: %c\n", _command->identifier);
	switch(_command->identifier)
	{
		case '\n':
DEBUG_PRINT(stderr, "Executing /n: %d\n", _current_address->address_value);
			list_print_to_stream(_head, stdout, _current_address->address_value + 1, 1, false);
			_current_address->address_value = _current_address->address_value + 1;
			_current_address->pre_quit = false;
			break;
		case 'H': 
			update_error_holder(_error, true, true);
			_current_address->pre_quit = false;
			break;
		case 'h': 
			update_error_holder(_error, true, false);
			_current_address->pre_quit = false;
			break;
		case 'q': 
			if (_current_address->modified_buffer)
			{
				if (_current_address->pre_quit)
				{
					_current_address->address_value = -1;	
				}
				else
				{
					_current_address->pre_quit = true;
					update_error_holder_code(_error, 9, false);
				}
			}
			else
			{
				_current_address->address_value = -1;				
			}
			break;
		case 'w': 
			write_command(_head, _error, _command, _current_address);
			_current_address->pre_quit = false;
			break;	
	}
}

//only fatal error returns fall, when quit is called address_holder.address_value sets value to negative number
bool process_user_command(file_holder* _file, head* _list, error_holder* _error, address_holder* _current_address, const char* _line)
{
	//ED IS TRASH 0 IS INVALID ADDRESS, and I am not going to fix it I will just put this hack here
	if (strcmp(_line, "0i\n") == 0)
	{
		_line = "1i\n";
	}
	
DEBUG_PRINT(stderr, "%s\n", "process_user_command::called");
	address n = ADDRESS_DEFAULT;
	address m = ADDRESS_DEFAULT;
	int command_offset = 0;
	bool result = obtain_address(&n , &m, &command_offset, _line);
DEBUG_PRINT(stderr, "%d\n", result);
	command_address* _command_address = determine_address_validity(&n, &m, command_offset, result, _current_address->address_value, _list->count + 1);

	if (_command_address == nullptr)
	{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::address::error");
		update_error_holder_code(_error, 2, false);
		return true; //no valid address
	}

	command* _command = get_command(_error ,_line + command_offset);
	if (_command == nullptr)
	{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::error::incorrect_command");
		free(_command_address);
		return true; //no valid command
	}

	if (_command->identifier == '\n' && _command_address->no_address_defined) //look another hack of life because ed is trash
	{
		_command->requires_address = false;

		if (_current_address->address_value >= _list->count) //incorrect function call
		{
			update_error_holder_code(_error, 2, false);
			free(_command_address);
			free(_command);
			return true; //no valid command
		}
	}


	if (_command->requires_arg)
	{
		/*if ((_command->arg)[0] == '\n')
		{
		fprintf(stderr, "%s\n", "debug hello");			
		}
		if ((_command->arg)[0] == '\0')
		{
		fprintf(stderr, "%s\n", "debug null");			
		}
		fprintf(stderr, "%s::%c::%s\n", "debug alfa", _command->identifier, _command->arg);
		fprintf(stderr, "%s::%d::%d\n", "debug alfa", _command->requires_address, _command->requires_arg);
		fflush(stderr);*/

		if ((_command->arg)[0] != '\0') // arg was given
		{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::handle::arg");
			if ((_file->file_name)[0] == '\0') //no file
			{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::late_save_file_name");
				strcpy(_file->file_name, _command->arg);
			}
		}
		if ((_command->arg)[0] == '\0') // no arg was given
		{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::handle::no_arg");
			if ((_file->file_name)[0] != '\0') //special case with file arg suffix
			{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::obtain_file_name_from_arg");
				strcpy(_command->arg, _file->file_name);
			}
			else
			{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::impossible_to_set_file_name");
				update_error_holder_code(_error, 4, false);
				free(_command_address);
				free(_command);
				return true; //no valid command
			}
		}
	} 

	if (_command->requires_address && _command_address->no_address_defined) //this is standard of this code because gnu ed is trash!
	{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::missing_address");
		update_error_holder_code(_error, 2, false); //invalid address
		free(_command_address);
		free(_command);
		return true;
	}
	if (!_command->requires_address && !_command_address->no_address_defined) 
	{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::unexpected_address");
		update_error_holder_code(_error, 3, false); //unexpected address
		free(_command_address); 
		free(_command);
		return true;
	}

	if (_command->requires_address && !_command_address->no_address_defined) //address is required for command
	{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::execute_address_command");
		execute_address_command(_list, _command, _error, _current_address, _command_address);
		free(_command);
		free(_command_address);
		return true;
	}
	
	if (!_command->requires_address && _command_address->no_address_defined) //address is not required for command
	{
DEBUG_PRINT(stderr, "%s\n", "process_user_command::execute_single_command");
		execute_single_command(_list, _command, _error, _current_address);
		free(_command);
		free(_command_address);
		return true;
	}
DEBUG_PRINT(stderr, "%s\n", "process_user_command::error11");

	//all other execution paths results in error
	free(_command);
	free(_command_address);
	return false; 
}

/*	Main program processor
	On critical failure returns >0.
	Otherwise returns 0.
*/
int process_user_input(head* _head, file_holder* _file, error_holder* _error)
{
DEBUG_PRINT(stderr, "%s\n", "process_user_input::called");
	address_holder current_address = ADDRESS_HOLDER_DEFAULT; 
	current_address.address_value = _head->count;	
	char *new_line = nullptr;
	size_t size = 0;
	ssize_t line_length;	
	int return_code = 0;
	bool is_running = true;
	//while (is_running)
	{
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::started_cycle");
		while (is_running && (line_length = getline(&new_line, &size, stdin)) != -1)
		{
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::new_line");
DEBUG_PRINT(stderr, "%s", new_line);
			if (line_length > LINE_LENGTH)
			{
				fprintf(stderr, "ed: line too long, limit: %d, read: %ld\n", LINE_LENGTH, line_length);
				fflush(stderr);
				is_running = false;
				return_code = 1;
				break;			
			}
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::command::began");
			if (process_user_command(_file, _head, _error, &current_address, new_line))
			{
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::command::success");
				if (current_address.address_value < 0) //command "q"
				{
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::command::quit");
					is_running = false;
				}
				if (_error->code == 8)
					continue;
				if (!(_file->error_code == 0) && _error->is_set) //if file opening throws error, code executes until another error is triggered
				{
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::command::force_quit");
					is_running = false;	
					return_code = 1;
				}
				if (_error->is_set) //why 8 ? exceptions from exceptions right ? just shut up ed you are drunk from birth
				{
					return_code = 1;
				}
			}
			else
			{
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::command::failed");
				is_running = false;
				return_code = 1;
			}
			fflush(stdout);
			fflush(stderr);
		}
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::finished_cycle");
	}
	free(new_line); 
DEBUG_PRINT(stderr, "%s\n", "process_user_input::processing::finished");
	if (errno != 0)
	{ 
		fprintf(stderr, "%s\n", strerror(errno));
		fflush(stderr);
		return_code = errno;
		errno = 0;
	}
DEBUG_PRINT(stderr, "%s\n", "process_user_input::finished");
	return return_code;
}

/*	Executes simplified GNU ED
	On correct execution exits with 0.
	In case of error detected by code exits with 1.
	If program called exit, all resources should be properly freed.
	In all other cases, behaviour is not specified. In best case will crash.
*/
int main(const int _argc, const char** _argv)
{
DEBUG_PRINT(stderr, "%s\n", "main::called");
	head main_file_list = HEAD_DEFAULT;
	file_holder main_file = FILE_HOLDER_DEFAULT;
	option_holder options = OPTION_HOLDER_DEFAULT;
	error_holder error = ERROR_HOLDER_DEFAULT;
	if (!parse_args(_argc, _argv, &options, &main_file))
	{
DEBUG_PRINT(stderr, "%s\n", "main::incorrect::args::parsed");
		exit(1);
	}

#if OPTIONS_UNWANTED
	if (options.is_set)
	{
DEBUG_PRINT(stderr, "%s\n", "main::incorrect::args::set");
		fprintf(stderr, "%s\n", "ed: options are not supported");
		fflush(stderr);
		fprintf(stderr, "h:%d, V:%d, G:%d, l:%d, r:%d, s:%d, v:%d, p:%d=%s\n", 
			options.help, options.version, options.traditional, options.loose_exit_status,
			options.restricted, options.silent, options.verbose, options.prompt, options.prompt_string);
		fflush(stderr);
		fprintf(stderr, "%s\n", "ed: illegal option usage");
		fflush(stderr);
		fprintf(stderr, "%s\n", "usage: ed file");	
		fflush(stderr);
		exit(1);
	} 
#endif

#if FILE_IS_MANDATORY
	if (main_file.file_name[0] == '\0') //demand file arg
	{
DEBUG_PRINT(stderr, "%s\n", "main::incorrect::args::no_file");
		fprintf(stderr, "%s\n", "ed: file is mandatory");
		fflush(stderr);
		fprintf(stderr, "%s\n", "usage: ed file");	
		fflush(stderr);
		exit(1);
	}
#endif	

DEBUG_PRINT(stderr, "%s\n", "main::file_handling");
	if (main_file.file_name[0] != '\0' && file_open(&main_file, &error)) //file was opened
	{
DEBUG_PRINT(stderr, "%s\n", "main::file_handling::opened");
		if (!load_file(&main_file, &main_file_list))
		{
DEBUG_PRINT(stderr, "%s\n", "main::file_handling::reading_error");
			list_destroy(&main_file_list);
DEBUG_PRINT(stderr, "%s\n", "main::list::cleared");
			file_close(&main_file);
DEBUG_PRINT(stderr, "%s\n", "main::file_handling::closed");
			exit(errno != 0 ? errno : 1);
		}
		else
		{
DEBUG_PRINT(stderr, "%s\n", "main::file_handling::reading_ok");
			file_close(&main_file); //attempt to close a file
DEBUG_PRINT(stderr, "%s\n", "main::file_handling::closed");
		}
	}
	else
	{
DEBUG_PRINT(stderr, "%s\n", "main::file_handling::unopenable_file");
	}

DEBUG_PRINT(stderr, "%s\n", "main::loop::called");
	int execution_result = process_user_input(&main_file_list, &main_file, &error);
DEBUG_PRINT(stderr, "%s\n", "main::loop::finished");

	list_destroy(&main_file_list); //we are nice and we clear most of our trash
DEBUG_PRINT(stderr, "%s\n", "main::list::cleared");
	return execution_result;
}

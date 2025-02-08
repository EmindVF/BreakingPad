build: sources
	mkdir -p objects
	nasm -f elf64 ./sources/cipher_caesar.asm -o ./objects/cipher_caesar.o
	nasm -f elf64 ./sources/cipher_substitution.asm -o ./objects/cipher_substitution.o
	nasm -f elf64 ./sources/cipher_transposition.asm -o ./objects/cipher_transposition.o
	nasm -f elf64 ./sources/decipher_caesar.asm -o ./objects/decipher_caesar.o
	nasm -f elf64 ./sources/decipher_substitution.asm -o ./objects/decipher_substitution.o
	nasm -f elf64 ./sources/decipher_transposition.asm -o ./objects/decipher_transposition.o
	nasm -f elf64 ./sources/read_char.asm -o ./objects/read_char.o
	nasm -f elf64 ./sources/read_file.asm -o ./objects/read_file.o
	nasm -f elf64 ./sources/write_file.asm -o ./objects/write_file.o
	g++ -c ./sources/main.cpp -o ./objects/main.o
	g++ -no-pie -o breakingpag ./objects/*.o
	rm -rf objects
	
clean: 
	rm -rf objects
	rm -f breakingpag
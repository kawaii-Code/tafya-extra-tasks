all: task7 fsa1.dot fsa2.dot fsa1.svg fsa2.svg fsa1_min.svg fsa2_min.svg

task7: extra-task-7.cpp
	c++ -ggdb -o task7 extra-task-7.cpp

fsa1.dot, fsa2.dot: task7
	./task7 fsa1.txt fsa2.txt

fsa1.svg: fsa1.dot
	dot -Tsvg fsa1.dot > fsa1.svg

fsa2.svg: fsa2.dot
	dot -Tsvg fsa2.dot > fsa2.svg

fsa1_min.svg: fsa1_min.dot
	dot -Tsvg fsa1_min.dot > fsa1_min.svg

fsa2_min.svg: fsa2_min.dot
	dot -Tsvg fsa2_min.dot > fsa2_min.svg

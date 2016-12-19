CC		:= gcc
FLAGS	:= --std=c99 -Wall -O3
SRC		:= src
BIN		:= bin
BUILD	:= .build

.PHONY: clean

all: clean $(BIN)/brr_decoder $(BIN)/spc_decoder $(BIN)/brr_encoder

$(BIN)/brr_decoder: $(BUILD)/brr.o $(SRC)/brr_decoder.c $(SRC)/common.h
	@mkdir -pv $(dir $@)
	$(CC) $(FLAGS) $(SRC)/brr_decoder.c $(BUILD)/brr.o -o $@

$(BIN)/spc_decoder: $(BUILD)/brr.o $(SRC)/spc_decoder.c $(SRC)/common.h
	@mkdir -pv $(dir $@)
	$(CC) $(FLAGS) $(SRC)/spc_decoder.c $(BUILD)/brr.o -o $@

$(BIN)/brr_encoder: $(BUILD)/brr.o $(SRC)/brr_encoder.c $(SRC)/common.h
	@mkdir -pv $(dir $@)
	$(CC) $(FLAGS) $(SRC)/brr_encoder.c $(BUILD)/brr.o -o $@

$(BUILD)/brr.o: $(SRC)/brr.h $(SRC)/brr.c $(SRC)/common.h
	@mkdir -pv $(dir $@)
	$(CC) $(FLAGS) -c $(SRC)/brr.c -o $@

clean:
	@rm -rf $(BUILD) $(BIN)

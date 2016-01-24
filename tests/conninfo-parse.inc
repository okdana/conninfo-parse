#!/bin/bash

conninfo-parse() {
	"${DDT_TEST_DIR}/../conninfo-parse" "${@}"
	return $?
}


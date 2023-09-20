package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"strings"
)

var cliName string = "ares "

func printPrompt() {
	fmt.Print(cliName, "> ")
}

// clearScreen clears the terminal screen
func clearScreen() {
	cmd := exec.Command("clear")
	cmd.Stdout = os.Stdout
	cmd.Run()
}

func cleanInput(text string) string {
	output := strings.TrimSpace(text)
	output = strings.ToLower(output)
	return output
}

func main() {
	reader := bufio.NewScanner(os.Stdin)
	printPrompt()
	for reader.Scan() {
		text := cleanInput(reader.Text())
		if strings.EqualFold("exit", text) {
			return
		} else {
			args := strings.Split(text, " ")

			command, err := IdentifyCommand(args[0])
			if err != nil {
				fmt.Printf("Error: %v\n", err)
			}

			argss, err := command.ExtractArgs(args)
			if err != nil {
				fmt.Printf("Error: %v\n", err)
			}

			fmt.Println(argss)
		}
		printPrompt()
	}
	fmt.Println()
}

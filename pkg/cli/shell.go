package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"strings"

	"github.com/torbenconto/ares"
)

var cliName string = "ares "
var env = ares.NewEnvironment()

func printPrompt() {
	fmt.Print(cliName, env.Pwd, " > ")
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
			out, e, err := env.Eval(text)
			if err != nil {
				fmt.Printf("Eval error: %v\n", err)
				return
			}
			env = e
			fmt.Println(out)
		}
		printPrompt()
	}
	fmt.Println()
}

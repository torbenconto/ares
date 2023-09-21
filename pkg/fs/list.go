package fs

import (
	"fmt"
	"os"
)

func ListDir(dir string) {
	files, err := os.ReadDir(dir)
	if err != nil {
		fmt.Printf("Error reading directory: %v", err)
		return
	}

	fmt.Printf("total: %d\n", len(files))
	for _, f := range files {
		fmt.Printf("%s\n", f)
	}
}

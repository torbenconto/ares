package fs

import "os"

func DirExists(dir string) bool {
	file, err := os.Stat(dir)
	if err != nil {
		return false
	}
	return file.IsDir()
}

package main

import (
	"log/slog"
	"net/http"
	"os"
	"os/exec"
	"strings"

	"github.com/go-chi/chi/v5"
)

func main() {
	router := chi.NewRouter()
	router.Method(http.MethodGet, "/*", http.FileServer(http.Dir("./web/public")))
	router.Post("/", func(w http.ResponseWriter, r *http.Request) {
		r.ParseForm()
		input := r.Form.Get("input")
		cmd := exec.Command("./logic")
		input = strings.ReplaceAll(input, "∧", "A")
		input = strings.ReplaceAll(input, "∨", "v")
		input = strings.ReplaceAll(input, "¬", "~")

		input = strings.TrimSpace(input)
		cmd.Stdin = strings.NewReader(input)
		output, _ := cmd.CombinedOutput()

		outputStr := strings.ReplaceAll(string(output), ">", "")

		w.Write([]byte(`<textarea id="output" rows="10" disabled >` + outputStr + `</textarea>`))

	})
	router.Get("/test", func(w http.ResponseWriter, r *http.Request) {

	})

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	slog.Info("server starting", "port", port)
	err := http.ListenAndServe(":"+port, router)
	slog.Error("server crashed", "error", err)
}

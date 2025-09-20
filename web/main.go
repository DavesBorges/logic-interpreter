package main

import (
	"net/http"

	"github.com/go-chi/chi/v5"
)

func main() {
	router := chi.NewRouter()
	router.Handle("/*", http.FileServer(http.Dir("./web/public")))
	router.Get("/test", func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte("It works"))
	})

	http.ListenAndServe(":8080", router)
}

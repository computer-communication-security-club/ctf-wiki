IMAGE := ctf-wiki-fsop
PORT  := 1337

.PHONY: run stop

run:
	docker build -t $(IMAGE) .
	docker run --rm -d --name $(IMAGE) -p $(PORT):1337 $(IMAGE)
	@echo "nc localhost $(PORT)"

stop:
	docker stop $(IMAGE)

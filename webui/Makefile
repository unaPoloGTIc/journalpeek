build:
	npm install
	ng build
push:
	ng build --prod --base-href="journalpeek"
	./node_modules/angular-cli-ghpages/angular-cli-ghpages -S --dir=./dist/webui/

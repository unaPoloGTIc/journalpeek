import { Component } from '@angular/core';
//import { ViewerComponent } from './viewer/viewer.component';
import { FormGroup, FormControl } from '@angular/forms';
import { JournalService } from './journal.service';
import { Page } from './page';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})

export class AppComponent {
    title = 'webui';
    logForm = new FormGroup({
	field: new FormControl(''),
	unique: new FormControl(''),
	regex: new FormControl(''),
	ignorecase: new FormControl('true'),
	pagesize: new FormControl('5')
  });
    cursor = '';
    lines = [];
    eof = false;
    constructor(public js: JournalService) { }

    getjournal(): void{this.js.getjournal().subscribe(l => {this.lines = l.items; this.cursor = l.cursor; this.eof = l.eof;});};
    ngOnInit() {
	this.getjournal();
  }

}

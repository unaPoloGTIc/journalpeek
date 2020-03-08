import { Component, OnInit } from '@angular/core';
import { FormGroup, FormControl } from '@angular/forms';
import { JournalService } from '../journal.service';

@Component({
  selector: 'app-viewer',
  templateUrl: './viewer.component.html',
  styleUrls: ['./viewer.component.css']
})
export class ViewerComponent implements OnInit {
    logForm = new FormGroup({
	field: new FormControl(''),
	unique: new FormControl(''),
	regex: new FormControl(''),
	ignorecase: new FormControl('true'),
	pagesize: new FormControl('5')
  });
    cursor = '';
    lines = [];
    constructor(public js: JournalService) { }

    ngOnInit() {
	//this.lines = this.js.getjournal();
  }

}

import { Component } from '@angular/core';
import { FormGroup, FormControl } from '@angular/forms';
import { JournalService } from './journal.service';
import { Page } from './page';
import { first } from 'rxjs/operators';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})

export class AppComponent {
    title = 'webui';
    field = new FormControl('');
    logForm = new FormGroup({
	unique: new FormControl(''),
	regex: new FormControl(''),
	ignorecase: new FormControl('true'),
	pagesize: new FormControl('50')
    });
    prevcur = '';
    cursor = '';
    lines = [];
    eof = false;
    fields = [];
    uniques = [];
    Convert = require('ansi-to-html');
    convert = new this.Convert();

    constructor(public js: JournalService) { }

    getjournal(backwards: boolean): void{
	const dir = backwards?this.prevcur:this.cursor;
	const tmp = this.js.getjournal(this.logForm, dir, backwards).pipe(first());
	const ret = tmp.subscribe(l => {this.lines = l.items;
					this.eof = false;
					if (backwards)
					{
					    this.cursor = this.prevcur;
					    this.prevcur = l.end;
					    this.lines.reverse();
					    if(l.eof)
					    {
						this.prevcur = "";
					    }
					}
					else
					{
					    this.prevcur = this.cursor;
					    this.cursor = l.end;
					    this.eof = l.eof;
					}
				       });
    };
    getfields(): void{this.js.getfields().subscribe(l => {this.fields = l;})};
    updateunique(): void {
	this.field.valueChanges.subscribe(v => {
	    this.js.getuniques(v).subscribe(l => {
		this.uniques = l;
		this.logForm.patchValue({unique: ""})});
	});
    };
    updateform(): void {
	this.logForm.valueChanges.subscribe(v => {
	    this.cursor = this.prevcur;
	    this.getjournal(false);
	});
    };
    ngOnInit() {
	this.getjournal(false);
	this.getfields();
	this.updateunique();
	this.updateform();
  }

}
//TODO: fix paging with regex

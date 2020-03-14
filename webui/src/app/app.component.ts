import { Component } from '@angular/core';
import { FormGroup, FormControl } from '@angular/forms';
import { JournalService } from './journal.service';
import { Page } from './page';
import { first } from 'rxjs/operators';
import { DomSanitizer } from '@angular/platform-browser';

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
    columnsToDisplay = ['Matches'];

    wait_for_fields = true;
    wait_for_lines = true;

    constructor(public js: JournalService, private sanitizer: DomSanitizer) { }

    getjournal(backwards: boolean): void{
	this.wait_for_lines = true;
	const dir = backwards?this.prevcur:this.cursor;
	const tmp = this.js.getjournal(this.logForm, dir, backwards).pipe(first());
	const ret = tmp.subscribe(l => {
	    this.lines = [];
	    l.items.forEach(
		(line) => {
		    const trusted = this
			.sanitizer
			.bypassSecurityTrustHtml(
			    this.convert.toHtml(line));
		    this.lines.push(trusted);});
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
	    this.wait_for_lines = false;
	});};

    getfields(): void{
	this.wait_for_fields = true;
	this.js.getfields().subscribe(l => {
	    this.fields = l;
	    this.wait_for_fields = false;})};

    updateunique(): void {
	this.field.valueChanges.subscribe(v => {
	    this.wait_for_fields = true;
	    this.js.getuniques(v).subscribe(l => {
		this.uniques = l;
		this.logForm.patchValue({unique: ""});
		this.wait_for_fields = false;});
	});
    };

    updateform(): void {
	this.logForm.valueChanges.subscribe(v => {
	    this.cursor = this.prevcur;
	    this.getjournal(false);
	});
    };

    ngOnInit() {
	this.getfields();
	this.updateunique();
	this.getjournal(false);
	this.updateform();
  }

}
//TODO: fix paging with regex

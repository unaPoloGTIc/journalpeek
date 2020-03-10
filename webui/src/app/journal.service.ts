import { Injectable } from '@angular/core';
import { Observable, of } from 'rxjs';
import { HttpClient } from '@angular/common/http';
import { Page } from './page';

@Injectable({
  providedIn: 'root'
})
export class JournalService {
    p: Page;
    jUrl = 'http://localhost:6666/v1/';
    
    getjournal(form: any, cur: string, back: boolean):Observable<Page>
    {
	let jdata = {
	    backwards: back,
	    pagesize: parseInt(form.value.pagesize),
	    begin: cur,
	    regex: form.value.regex,
	    ignore_case: form.value.ignorecase,
	};
	if (form.value.unique !== "")
	{
	    jdata['match'] = form.value.unique;
	}
	
	return this.http.post<Page>(this.jUrl + 'paged_search', jdata);
	//return of(this.p);
    }
    getfields():Observable<string[]>
    {
	return this.http.get<string[]>(this.jUrl + 'all_fields');
    }
    getuniques(f: string):Observable<string[]>
    {
	return this.http.post<string[]>(this.jUrl + 'field_unique', {field:f});
    }
    constructor(private http: HttpClient) {
	this.p = {items : ['line1','line2','line3'],
		  eof : true,
		  end : "deadMarker"};
    }
}

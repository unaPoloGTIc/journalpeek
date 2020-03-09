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
    
    getjournal():Observable<Page>
    {
	return this.http.get<Page>(this.jUrl + 'paged_search');
	//return of(this.p);
    }
    getfields():Observable<string[]>
    {
	return this.http.get<string[]>(this.jUrl + 'all_fields');
    }
    getuniques(f: string):Observable<string[]>
    {
	console.log("dammit cors")
	//return this.http.get<string[]>(this.jUrl + 'field_unique', {headers: {"Content-Type":"application/json"}, responseType: "json", observe: "body"});
	return this.http.post<string[]>(this.jUrl + 'field_unique', {field:f});
    }
    constructor(private http: HttpClient) {
	this.p = {items : ['line1','line2','line3'],
		  eof : true,
		  end : "deadMarker"};
    }
}

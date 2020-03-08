import { Injectable } from '@angular/core';
import { Observable, of } from 'rxjs';
import { Page } from './page';

@Injectable({
  providedIn: 'root'
})
export class JournalService {
    p: Page;
    getjournal():Observable<Page>
    {return of(this.p);}
    
    constructor() {
	this.p = {items : ['line1','line2','line3'],
		  eof : true,
		  cursor : "deadMarker"};
    }
}

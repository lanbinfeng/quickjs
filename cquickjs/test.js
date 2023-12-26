import {boops} from './test2.js'


class Blub {
    constructor() {
        this.peeps = 3;
    }
    jib() {
	    let index = 0;
	    while (true)
	    {
		    index++;
        	    console.log(index);
        	    console.log("_____1________");
        	    console.log("_____1________");
	    }
    }
}

var blub = new Blub();
blub.jib();

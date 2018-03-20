"use strict";

(function() {
	// the API end point
	var url = "getListOfFavPlaces";

	// TO DO: YOU NEED TO USE AJAX TO CALL getListOfFavPlaces end-point from server
	// STEPS:
	// 1. Hit the getListOfFavPlaces end-point of server using AJAX get method
	// 2. Upon successful completion of API call, server will return the list of places
	// 2. Use the response returned to dynamically add rows to 'myFavTable' present in favourites.html page
	// 3. You can make use of jQuery or JavaScript to achieve this
	// Note: No changes will be needed in favourites.html page
	$.getJSON(url, function( data ) {
	  var items = [];
		//console.log(data);
		//console.log(data["res"]);
		var placeList = data["res"];
		var places = placeList["placeList"];
		$.each( places, function( num, value ) {
			var onerow = "<tr>";

			onerow = onerow + "<th>" + value["placename"] + "</th>";
			onerow = onerow + "<th>" + value["addressline1"] + value["addressline2"] + "</th>";
			onerow = onerow + "<th>" + value["opentime"] + "/" + value["closetime"] + "</th>";
			onerow = onerow + "<th>" + value["additionalinfo"] + "</th>";
			onerow = onerow + "<th>" + value["additionalinfourl"] + "</th></tr>";
		  console.log(onerow);
			items.push(onerow);
		});

		$("tbody").append(items.join(""));
	});
})();

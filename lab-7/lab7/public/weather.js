$(document).ready(function() {
	var cityfield=$("#cityfield")
	cityfield.keyup(function() {
		var url = "/getcity?q=" + cityfield.val();
		$.getJSON(url,function(data){
			var everything;
			everything = "<ul>";
			$.each(data, function(i,item){
				everything+="<li> " +data[i].city;
			});
				everything+="</ul>";
				$("#txtHint").html(everything);
			});
		});
		$("#button").click(function(e){
			e.preventDefault()
			var value = cityfield.val();
			$("#dispcity").text(value);
			var myurl= "https://api.wunderground.com/api/383c8d4b5e128d0d/geolookup/conditions/q/UT/";
			  myurl += value;
			  myurl += ".json";
			  console.log(myurl);
		  $.ajax({
		    url : myurl,
		    dataType : "jsonp",
		    success : function(data) {
			var location = data['location']['city']
			var temp_string = data['current_observation']['temperature_string']
			var current_weather=data['current_observation']['weather']
			var everything="<ul>"
				everything += "<li>Location: "+location
				everything += "<li>Temperature: "+temp_string
				everything += "<li>Weather: "+current_weather
				everything += "</ul>"
			$("#weather").html(everything)
		        console.log(data);
		    }
		})
		$.ajax({
			url:'/joke',
			success : function(data){
				$("#joke").html(data)
			}
		})
	})
})

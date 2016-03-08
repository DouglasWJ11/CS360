var fs = require('fs')
var express = require('express');
var bodyParser = require('body-parser');

var app = express();

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));
app.use(express.static(path.join(__dirname, 'public')));
var cities =[]
app.get("/getcity", function(req, res){
	var prefix = req.query.q
	var arr =  cities.reduce(function (arr,city){
		if(city.substring(prefix.length,0)===prefix){
			arr.push({'city': city})
		}
		return arr
	},[])
	res.send(arr)
})

app.get('/joke', function(req,res){
	request('http://api.icndb.com/jokes/random', function(err, response, body){
		var datJSON.parse(body)
		res.send(data.value.joke)
		})
}) 
fs.readFile('./staticCity.txt', 'utf8', function(err, data){
	cities=data.split('\n')
})
app.listen(3000)

// catch 404 and forward to error handler
app.use(function(req, res, next) {
  var err = new Error('Not Found');
  err.status = 404;
  next(err);
});

// error handlers

// development error handler
// will print stacktrace
if (app.get('env') === 'development') {
  app.use(function(err, req, res, next) {
    res.status(err.status || 500);
    res.render('error', {
      message: err.message,
      error: err
    });
  });
}

// production error handler
// no stacktraces leaked to user
app.use(function(err, req, res, next) {
  res.status(err.status || 500);
  res.render('error', {
    message: err.message,
    error: {}
  });
});



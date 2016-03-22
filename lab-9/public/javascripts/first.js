angular.module('comment', [])
.controller('MainCtrl', [
  '$scope','$http',
  function($scope,$http){
    $scope.test = 'Hello world!';
    $scope.comments=[]
    $scope.addComment = function(){
	if($scope.formContent===''){return}
	$scope.create({
		title: $scope.formContent,
		upvotes: 0,
	});
	$scope.formContent='';
    }
    $scope.incrementUpvotes = function(comment) {
    	$scope.upvote(comment);  
    }
    $scope.decrementUpvotes=function(comment){
    	$scope.downvote(comment); 
    }
    $scope.getAll = function() {
    return $http.get('/comments').success(function(data){
      angular.copy(data, $scope.comments);
    });
  };
    $scope.create=function(comment){
	return $http.post('/comments', comment).success(function(data){
	$scope.comments.push(data);
	});
    }
    $scope.upvote=function(comment){
	return $http.put('/comments/' + comment._id + '/upvote')
		.success(function(data){
		comment.upvotes =+ 1;
	});
    }
    $scope.downvote=function(comment){
	return $http.put('/comments/' + comment._id+ '/downvote')
		.success(function(data){
		comment.upvotes -= 1;
	})
    }
  }
])

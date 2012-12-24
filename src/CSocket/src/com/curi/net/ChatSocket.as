package com.curi.net {
	import flash.events.Event;
	import flash.events.IOErrorEvent;
	import flash.events.ProgressEvent;
	import flash.events.SecurityErrorEvent;
	import flash.net.Socket;
	import flash.utils.ByteArray;
	import flash.external.ExternalInterface;

	public class ChatSocket {

		private var _socket:Socket;
		private var _host:String;
		private var _port:int = -1;

		public function ChatSocket( host:String, port:int ) {
			_host = host;
			_port = port;
			_socket = new Socket();

			_socket.addEventListener( Event.CLOSE, closeHandler );
			_socket.addEventListener( Event.CONNECT, connectHandler );
			_socket.addEventListener( IOErrorEvent.IO_ERROR, ioErrorHandler );
			_socket.addEventListener( SecurityErrorEvent.SECURITY_ERROR, securityErrorHandler );
			_socket.addEventListener( ProgressEvent.SOCKET_DATA, socketDataHandler );
		}

		public function dispose():void {
			_host = null;
			_port = -1;

			_socket.removeEventListener( Event.CLOSE, closeHandler );
			_socket.removeEventListener( Event.CONNECT, connectHandler );
			_socket.removeEventListener( IOErrorEvent.IO_ERROR, ioErrorHandler );
			_socket.removeEventListener( SecurityErrorEvent.SECURITY_ERROR, securityErrorHandler );
			_socket.removeEventListener( ProgressEvent.SOCKET_DATA, socketDataHandler );
			_socket = null;
		}

		public function join( name:String ):void {
			debugTrace( 'join' );
			_socket.connect( _host, _port );
		}

		public function leave():void {
			debugTrace( 'leave' );
			_socket.close();
			//this.dispose();
		}

		public function sendMessage( body:String ):void {
			debugTrace( "message sending...", body );
			var toBody:String   = body + "¥r¥n";
			var bytes:ByteArray = new ByteArray();
			bytes.writeUTF( toBody );
			_socket.writeBytes( bytes );
			_socket.flush();
		}

		private function socketDataHandler( event:ProgressEvent ):void {
			debugTrace( "socketDataHandler: " + event );
			var bytes:ByteArray = new ByteArray();
			_socket.readBytes( bytes, event.bytesTotal );
			debugTrace( bytes.toString());
			//var body:String = _socket.readUTF();
			//trace( body );

		}

		private function connectHandler( event:Event ):void {
			debugTrace( "connectHandler: " + event );
		}

		private function closeHandler( event:Event ):void {
			debugTrace( "closeHandler: " + event );
			dispose();
		}

		private function ioErrorHandler( event:IOErrorEvent ):void {
			debugTrace( "ioErrorHandler: " + event );
		}

		private function securityErrorHandler( event:SecurityErrorEvent ):void {
			debugTrace( "securityErrorHandler: " + event );
		}

		private function debugTrace( ... args ):void {
			var str:String = args.join( ' ' );
			trace( str );
			ExternalInterface.call( 'console.log', str );
		}

	}
}